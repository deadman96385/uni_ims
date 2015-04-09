/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30336 $ $Date: 2014-12-11 10:24:15 +0800 (Thu, 11 Dec 2014) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>

#include <ezxml.h>
#include <auth_b64.h>
#include <milenage.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>
#include <sip_auth.h>
#include <sip_sdp_msg.h>

#include "isi.h"
#include "isi_errors.h"
#include "isip.h"

#include "mns.h"

#ifdef INCLUDE_SIMPLE
#include "xcap.h"
#include "_simple_types.h"
#endif

#include "_sapp.h"
#include "_sapp_ipsec.h"
#include "_sapp_dialog.h"
#include "_sapp_reg.h"
#include "_sapp_mwi.h"
#include "_sapp_te.h"
#include "_sapp_ussd.h"
#include "_sapp_parse_helper.h"
#include "_sapp_xml.h"
#include "_sapp_im_page.h"
#include "_sapp_call_settings.h"
#include "_sapp_coder_helper.h"
#include "_sapp_radio_update.h"
#include "_sapp_capabilities.h"
#include "_sapp_emergency.h"
#include "_sapp_conf.h"

#include <ims_net.h>
#include <sr.h>

#ifdef INCLUDE_SIMPLE
#include "_simple.h"
#include "simple/_simple_types.h"
#endif

#include <settings.h>

const char* SAPP_GetResponseReason(int code) {
    tSipMsgCodes intCode;
    intCode = MSGCODE_GetInt(code);
    if (intCode != eSIP_RSP_LAST_RESPONSE_CODE) {
        return (MSGCODE_GetStr(intCode));
    }
    return "Unknown";
}

ISI_Id SAPP_getUniqueIsiId(ISI_Id serviceId)
{
    static uint32 uniqueId  = 0;
    uint32 id;
    uniqueId++;
    if (uniqueId > SAPP_MAX_ID) {
        uniqueId = 1;
    }
    id = (uint32)serviceId;
    id = (id << 24);
    return ((ISI_Id)(uniqueId | id));
}

/*
 * ======== SAPP_serviceIsiEvt() ========
 *
 * This function is used by various other functions to populate a ISI event
 * for "UA/Service" events. These events will be passed from SAPP
 * to the ISI module.
 *
 * Returns:
 * Nothing.
 */
void SAPP_serviceIsiEvt(
    ISI_Id              serviceId,
    vint                protocolId,
    ISIP_ServiceReason  reason,
    ISIP_Status         status,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = serviceId;
    isi_ptr->code = ISIP_CODE_SERVICE;
    isi_ptr->protocol = protocolId;
    isi_ptr->msg.service.reason = reason;
    isi_ptr->msg.service.status = status;
    return;
}


/*
 * ======== SAPP_setHostAndPort() ========
 * This function converts a string in the format <address>:<port>
 * for example "talk.gmail.com:5322" and separates the address and port into
 * separate fields
 *
 * Returns:
 *  Nothing
 */
void SAPP_setHostAndPort(
    char     *host_ptr,
    char     *name_ptr,
    vint      maxNameLen,
    uint16   *port_ptr)
{
    char *curr_ptr;
    vint  len;

    /* check host_ptr if it is IPv6 string */
    if ((NULL != OSAL_strscan(host_ptr, "[")) &&
            (NULL != OSAL_strscan(host_ptr, "]"))) {
        /* IPv6 */
        host_ptr = OSAL_strscan(host_ptr, "[");
        curr_ptr = OSAL_strscan(host_ptr, "]");
        len = (curr_ptr - host_ptr -1);
        if (len > maxNameLen) {
            len = maxNameLen;
        }
        OSAL_memCpy(name_ptr, (host_ptr + 1), len);
        /* Make sure we NULL terminate, function callers
         * make sure there's enough on the end to write NULL
         */
        name_ptr[len] = 0;
        /* See if ':' exists, if so then there is a port.
         * NOTE, host_ptr must be null terminated
         */
        curr_ptr = OSAL_strscan(curr_ptr, ":");
        if (curr_ptr) {
            /* Now get the port */
            curr_ptr++;
            *port_ptr = (uint16)OSAL_atoi(curr_ptr);
        }
        else {
            /* no port so init to zero */
            *port_ptr = 0;
        }
    }
    else {
        /* NOT IPv6 */
        /* See if ':' exists, if so then there is a port.
         * NOTE, host_ptr must be null terminated
         */
        curr_ptr = OSAL_strscan(host_ptr, ":");
        if (curr_ptr) {
            /* We have a port */
            len = curr_ptr - host_ptr;
            if (len > maxNameLen) {
                len = maxNameLen;
            }
            OSAL_memCpy(name_ptr, host_ptr, len);
            /* Make sure we NULL terminate, function callers
             * make sure there's enough on the end to write NULL
             */
            name_ptr[len] = 0;
            /* Now get the port */
            curr_ptr++;
            *port_ptr = (uint16)OSAL_atoi(curr_ptr);
        }
        else {
            /* There is no port so just copy */
            len = OSAL_strlen(host_ptr);
            if (len > maxNameLen) {
                len = maxNameLen;
            }
            OSAL_memCpy(name_ptr, host_ptr,len);
            /* Make sure we NULL terminate, function callers
             * make sure there's enough on the end to write NULL
             */
            name_ptr[len] = 0;
            /* no port so init to zero */
            *port_ptr = 0;
        }
    }

}

/*
 * ======== _SAPP_insertBlockedUser() ========
 *
 * This function inserts a NULL terminated string containing a SIP URI
 * of a user that the user wishes to block (deny service to).
 *
 * Returns:
 *   SAPP_ERR : Could not insert the user (URI) into the list. List is full.
 *   SAPP_OK : User was successfully added to list.
 */
static vint _SAPP_insertBlockedUser(
    SAPP_BlockUsers *block_ptr,
    char            *uri_ptr)
{
    vint x;
    /* Search for an empty spot in the array and insert */
    for (x = 0 ; x < block_ptr->maxUsers ; x++) {
        if (block_ptr->aUsers[x][0] == 0) {
            /* Found one */
            OSAL_snprintf(block_ptr->aUsers[x], SAPP_STRING_SZ,
                    "%s", uri_ptr);
            return (SAPP_OK);
        }
    }
    return (SAPP_ERR);
}

/*
 * ======== SAPP_searchBlockedUser() ========
 *
 * This function will search to see if a particular user's URI
 * is in the list of "Blocked" users.  If a user (URI) is found in the list
 * then that means that we should deny this user access to this device.
 *
 * This function can also be used to search and remove a user.  For example,
 * if the user of this device wishes to allow another remote user to contact
 * us.
 *
 * Returns:
 *   SAPP_ERR : Could not find the user (URI) in the list.
 *   SAPP_OK : User was successfully found (and possibly removed).
 */
vint SAPP_searchBlockedUser(
    SAPP_BlockUsers *block_ptr,
    char            *uri_ptr,
    vint             shouldRemove)
{
    vint x;
    /* See if the URI exists in the list */
    for (x = 0 ; x < block_ptr->maxUsers ; x++) {
        if (OSAL_strncmp(block_ptr->aUsers[x], uri_ptr,
                SAPP_STRING_SZ) == 0) {
            /* Found it */
            if (shouldRemove == 1) {
                block_ptr->aUsers[x][0] = 0;
            }
            return (SAPP_OK);
        }
    }
    return (SAPP_ERR);
}

/*
 * ======== SAPP_findServiceViaServiceId() ========
 *
 * This function will search for a service that has an ID as specified in
 * "serviceId".  If a service is found that has a matching "serviceId" then
 * a pointer to the service object is returned.  IF the serviceId can not be
 * found then NULL is returned.
 *
 * Returns:
 *   SAPP_ServiceObj *: A pointer to the service object that has the same
 *                 service ID as "serviceId"
 *   NULL : The service specified by serviceId does not exist.
 */
SAPP_ServiceObj* SAPP_findServiceViaServiceId(
    SAPP_SipObj *sip_ptr,
    ISI_Id     serviceId)
{
    vint x;

    for (x = 0 ; x < SAPP_SIP_MAX_UA ; x++) {
        if (sip_ptr->service[x].isiServiceId == serviceId) {
            return (&sip_ptr->service[x]);
        }
    }
    return (NULL);
}

/*
 * ======== SAPP_findServiceViaUaId() ========
 *
 * This function will search for a service that has a UA ID as specified in
 * "uaId".  If a service is found that has a matching "uaId" then
 * a pointer to the service object is returned.  IF the uaId can not be
 * found then NULL is returned.
 *
 * Returns:
 *   SAPP_ServiceObj *: A pointer to the service object that has the same
 *                 UA ID as "uaId"
 *   NULL : The service specified by serviceId does not exist.
 */
static SAPP_ServiceObj* _SAPP_findServiceViaUaId(
    SAPP_SipObj *sip_ptr,
    tSipHandle uaId)
{
    vint x;

    for (x = 0 ; x < SAPP_SIP_MAX_UA ; x++) {
        if (sip_ptr->service[x].sipConfig.uaId == uaId) {
            return (&sip_ptr->service[x]);
        }
    }
    return (NULL);
}

/*
 * ======== SAPP_findCallViaIsiId() ========
 *
 * This function will search for a call with in the service specified by
 * service_ptr. If a call was found that matches the call ID specified
 * in "callId" then a pointer to the call object is returned.
 * Otherwise if the callId can not be found then NULL is returned.
 *
 * Returns:
 *   SAPP_CallObj *: A pointer to the call object that has the same
 *                      call ID as "callId"
 *   NULL : The call specified by callId does not exist.
 */
SAPP_CallObj* SAPP_findCallViaIsiId(
    SAPP_ServiceObj *service_ptr,
    ISI_Id              callId)
{
    vint x;

    for (x = 0 ; x < SAPP_CALL_NUM ; x++) {
        if (service_ptr->sipConfig.aCall[x].isiCallId == callId) {
            return (&service_ptr->sipConfig.aCall[x]);
        }
    }
    return (NULL);
}

/*
 * ======== SAPP_findCallViaDialogId() ========
 *
 * This function will search for a call with in the service specified by
 * service_ptr. If a call was found that matches the dialog ID specified
 * in "dialogId" then a pointer to the call object is returned.
 * Otherwise if the callId can not be found then NULL is returned.
 *
 * Returns:
 *   SAPP_CallObj *: A pointer to the call object that has the same
 *                      dialog ID as "dialogId"
 *   NULL : The call specified by dialogId does not exist.
 */
SAPP_CallObj* SAPP_findCallViaDialogId(
    SAPP_ServiceObj *service_ptr,
    tSipHandle          dialogId)
{
    vint x;

    for (x = 0 ; x < SAPP_CALL_NUM ; x++) {
        /*
         * Upon receiving error response to INVITE, it will destroy dialog
         * in SIP and clear dialog id in SAPP. In this case, the dialogId is 0
         * but the isiCallId isn't. It need to check isiCallId to allocate
         * a new call.
         */
        if (service_ptr->sipConfig.aCall[x].dialogId == dialogId) {
            if ((0 == dialogId) &&
                    (0 != service_ptr->sipConfig.aCall[x].isiCallId)) {
                continue;
            }
            return (&service_ptr->sipConfig.aCall[x]);
        }
    }
    return (NULL);
}

/*
 * ======== SAPP_findCallConfEventViaDialogId() ========
 *
 * This function will search for a conf dialog with in the service specified by
 * service_ptr. If a conf dialog was found that matches the dialog ID specified
 * in "dialogId" then a pointer to the associated call object is returned.
 * Otherwise if the callId can not be found then NULL is returned.
 *
 * Returns:
 *   SAPP_CallObj *: A pointer to the call object that has the same
 *                      conference dialog ID as "dialogId"
 *   NULL : The call specified by dialogId does not exist.
 */
SAPP_CallObj* SAPP_findCallConfEventViaDialogId(
    SAPP_ServiceObj *service_ptr,
    tSipHandle       dialogId)
{
    vint x;

    for (x = 0 ; x < SAPP_CALL_NUM ; x++) {
        if ((service_ptr->sipConfig.aCall[x].conf.dialogId == dialogId) ||
                (service_ptr->sipConfig.aCall[x].dialogId == dialogId)) {
            return (&service_ptr->sipConfig.aCall[x]);
        }
    }
    return (NULL);
}

/*
 * ======== _SAPP_sipServiceQValue() ========
 *
 * This function is to set the q value for the "service"
 * specified in service_ptr.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceQValue(
    SAPP_ServiceObj *service_ptr,
    char            *q_ptr)
{
    OSAL_snprintf(service_ptr->qValue, SIP_Q_VALUE_STR_SIZE, "%s",
            q_ptr);
    return;
}

/*
 * ======== _SAPP_sipServiceInit() ========
 *
 * This function is used to initialize a service object.  All internal object
 * members will be reset to a good initial state.  This function is typically
 * called when ISI has instructed SAPP to create a service.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceInit(
    SAPP_SipObj      *sip_ptr,
    SAPP_ServiceObj  *service_ptr,
    ISI_Id            id,
    ISIP_Service     *s_ptr,
    vint              protocolId)
{
    vint       x;
    tUaConfig *config_ptr;

    service_ptr->isiServiceId = id;
    service_ptr->blockCid = 0;
    service_ptr->stunAddr[0] = 0;
    service_ptr->stunPort = 0;
    service_ptr->ringTemplate = sip_ptr->ringTemplate;
    service_ptr->akaAuthRespSet = 0;
    service_ptr->confKeepAlive = sip_ptr->presence.keepAlive;
    service_ptr->protocolId = protocolId;
    service_ptr->natKeepaliveEnable = sip_ptr->keepAliveEnable;
    service_ptr->phoneContext[0] = 0;
    service_ptr->isNoNet = OSAL_TRUE;

    /* Initialize MNS service */
    MNS_initService(&service_ptr->mnsService, sip_ptr->usePrecondition);

    service_ptr->sipConfig.localConn.rport = 1;
    OSAL_netAddrPortCpy(&service_ptr->sipConfig.localConn.addr, &service_ptr->sipInfc);
    service_ptr->sipConfig.localConn.fd = service_ptr->sipInfcFd;

    SAPP_regInit(&service_ptr->registration, sip_ptr->keepAlive,
            sip_ptr->refresh, sip_ptr->regRetryBaseTime, 
            sip_ptr->regRetryMaxTime, sip_ptr->useRegEvt, NULL, service_ptr, 
            sip_ptr->queue.tmrEvt);

    SAPP_mwiInit(&service_ptr->mwiObj, sip_ptr->mwiRefresh, 
            sip_ptr->useMwiEvt, service_ptr);

    SAPP_teInit(&service_ptr->telEvt);
    SAPP_emgcyInit(&service_ptr->emergencyObj, sip_ptr->queue.tmrEvt);
    SAPP_imPageInit(&service_ptr->im);

    /*
     * Set the default VDN (VCC Domain Number) values for this service.
     */
    OSAL_snprintf(service_ptr->vdn, SAPP_STRING_SZ, "%s", sip_ptr->vdn);

    /* Use the 'name' for the "UserAgent" header field value */
    OSAL_snprintf(service_ptr->userAgent, SAPP_STRING_SZ, "%s", sip_ptr->name);

    /*
     * Set up the defaults for capabilities discovery.
     */
    service_ptr->capDiscoveryMethod = sip_ptr->capDiscoveryMethod;
    service_ptr->capDiscoveryViaCommonStack = sip_ptr->capDiscoveryViaCommonStack;

    service_ptr->regCapabilitiesBitmap = sip_ptr->regCapabilitiesBitmap;

    service_ptr->exchangeCapabilitiesBitmap =
            sip_ptr->exchangeCapabilitiesBitmap;

    service_ptr->srvccCapabilitiesBitmap = sip_ptr->srvccCapabilitiesBitmap;

    /* Clear the callForward field */
    service_ptr->callForward[0] = 0;

    /* Clear the ID (or handle in SIP terms) to '0' */
    service_ptr->sipConfig.uaId = 0;

    /* Init the database used for 'blocked users' */
    service_ptr->blockedUsers.maxUsers = SAPP_MAX_BLOCKED_USERS;
    for (x = 0 ; x < SAPP_MAX_BLOCKED_USERS ; x++) {
        service_ptr->blockedUsers.aUsers[x][0] = 0;
    }

    /* Init the object used for inbound & outbound presence subscriptions */
#ifdef INCLUDE_SIMPLE
    SIMPL_init(&service_ptr->simple, service_ptr, sip_ptr);
#endif

    config_ptr = &service_ptr->sipConfig.config;
    /* Zero out the entire SIP config object */
    OSAL_memSet(config_ptr, 0, sizeof(tUaConfig));

    /*
     * Set up the prack field that indicates if this service
     * should for the use of SIP's prack
     */
    service_ptr->sipConfig.usePrack = sip_ptr->usePrack;

    /*
     * Set up the cpim field that indicates if this service
     * should encode outbound page mode SMS's in CPIM format.
     */
    service_ptr->sipConfig.useCpim = sip_ptr->useCpim;

    /*
     * Set up the IPSEC field that indicates if this service
     * should for the use of SIP's IPSEC
     */
    service_ptr->sipConfig.useIpSec = sip_ptr->useIpSec;

    /*
     * Set up the capabilities value tha thte SIP stack will use to respond to
     * capability exchanges.
     */
    service_ptr->sipConfig.config.capabilitiesBitmap =
            sip_ptr->exchangeCapabilitiesBitmap;

    /*
     * Set up the isim field that indicates if isim is presented.
     */
    service_ptr->useIsim = sip_ptr->useIsim;

    /*
     * Set up the default and protected sip port.
     */
    service_ptr->ipsecObj.defaultPort = service_ptr->defaultPort;

    /*
     * Set up the session timer field that indicates if this service
     * should enforce rfc4028.
     */
    OSAL_snprintf(service_ptr->sipConfig.sessionTimer,
            SAPP_STRING_SZ, "%s", sip_ptr->sessionTimer);

    /*
     * Set up the force session timer field that indicates if this service
     * should enforce the use of session timer on MT side.
     */
    OSAL_snprintf(service_ptr->sipConfig.forceSessionTimer,
            SAPP_STRING_SZ, "%s", sip_ptr->forceSessionTimer);
    /*
     * Set up the q value.
     */
    _SAPP_sipServiceQValue(service_ptr , sip_ptr->qValue);
    
    /* Objects used for calls */
    for (x = 0 ; x < SAPP_CALL_NUM ; x++) {
        OSAL_memSet(&service_ptr->sipConfig.aCall[x], 0,
                sizeof(SAPP_CallObj));
        /* Init the modify state */
        service_ptr->sipConfig.aCall[x].modify = SAPP_MODIFY_NONE;
    }

    /*
     * The task Id is an OSAL message queue ID used for getting SIP events
     * for this service (or sip UA).
     */
    config_ptr->msgQId = sip_ptr->queue.sipEvt;

    /* Tell ISI that the service was successfully created */
    SAPP_serviceIsiEvt(id, service_ptr->protocolId, ISIP_SERVICE_REASON_CREATE,
                    ISIP_STATUS_DONE, &sip_ptr->event.isiMsg);
    return;
}

/*
 * ======== _SAPP_sipServiceDestroy() ========
 *
 * This function will mark a service object as "destroyed" by clearing the
 * serviceId member of the service object.  It will also destroy the underlying 
 * SIP User Agent. This function is typically called when ISI has instructed
 * SAPP to destroy a service.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceDestroy(
    SAPP_ServiceObj *service_ptr)
{
    /* Destroy the UA reset the isiServiceId */
    service_ptr->isiServiceId = 0;
    if (0 != service_ptr->sipConfig.uaId) {
        UA_Destroy(service_ptr->sipConfig.uaId);
        service_ptr->sipConfig.uaId = 0;
    }

    /* Socket sockets */
    SAPP_sipServiceTransportDestroy(service_ptr);

    /* Destroy emergency object */
    SAPP_emgcyDestroy(&service_ptr->emergencyObj);

    /* Destroy registration object */
    SAPP_regDestroy(&service_ptr->registration);    
    return;
}

/*
 * ======== _SAPP_sipServiceServer() ========
 *
 * This function is called when ISI has commanded SAPP to set a "server"
 * value for the "service" specified in service_ptr. It is considered
 * the 'command handler' for ISI commands pertaining to server settings.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceServer(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    ISIP_Service    *s_ptr)
{
    char *pos_ptr;
    char  proxyAddrStr[SAPP_STRING_SZ];
    char *str_ptr;
    char *tmp_ptr;
    int   i;

    switch (s_ptr->server) {
    case ISI_SERVER_TYPE_STUN:
        SAPP_setHostAndPort(s_ptr->settings.server,
                service_ptr->stunAddr, SAPP_STRING_SZ,
                &service_ptr->stunPort);
        if (service_ptr->stunAddr[0] == 0 || service_ptr->stunPort == 0) {
            /*
             * Then stun is being disabled over all.  So also reset the
             * sip interface info because it may have changed during a
             * previous registration.
             */
            OSAL_netAddrPortCpy(&service_ptr->sipConfig.localConn.addr, &service_ptr->sipInfc);
        }
        break;
    case ISI_SERVER_TYPE_PROXY:
        OSAL_snprintf(service_ptr->sipConfig.config.szProxy,
            SIP_URI_STRING_MAX_SIZE, "%s", s_ptr->settings.server);

        /* Remove the "sip:" scheme so that we could get correct port for ipsec. */
        pos_ptr = OSAL_strncasescan(s_ptr->settings.server,
                ISI_ADDRESS_STRING_SZ, "sip:");
        if (NULL == pos_ptr) {
            pos_ptr = s_ptr->settings.server;
        }
        else {
            /* Advance off the "sip:" */
            pos_ptr += 4;
        }

        /* Store proxy address and port for restoring from ipsec */
        SAPP_setHostAndPort(pos_ptr,
                proxyAddrStr, SAPP_STRING_SZ,
                &service_ptr->ipsecObj.defaultProxyPort);
        /*
         * When this command is received it means "no outbound proxy".
         * So also clear the 'outbound' proxy field.
         */
        service_ptr->registration.pcscfList[0][0] = 0; 
        service_ptr->sipConfig.config.szOutboundProxy[0] = 0;
        if (service_ptr->sipConfig.uaId != 0) {
            /* Then UA is currently active so call UA_Modify() */
            UA_Modify(service_ptr->sipConfig.uaId,
                    service_ptr->sipConfig.config.szProxy,
                    service_ptr->sipConfig.config.szOutboundProxy, NULL,
                    NULL, NULL, NULL, NULL, NULL, 20, 0);
        }
        break;
    case ISI_SERVER_TYPE_REGISTRAR:
        OSAL_snprintf(service_ptr->sipConfig.config.szRegistrarProxy,
            SIP_URI_STRING_MAX_SIZE, "%s", s_ptr->settings.server);
        if (service_ptr->sipConfig.uaId != 0) {
            /* Then UA is currently active so call UA_Modify() */
            UA_Modify(service_ptr->sipConfig.uaId,
                    NULL, 0, NULL, service_ptr->sipConfig.config.szRegistrarProxy,
                    NULL, NULL, NULL, NULL, 20, 0);
        }
        break;
    case ISI_SERVER_TYPE_OUTBOUND_PROXY:
        str_ptr = s_ptr->settings.server;
        /* Seperate and store multiple p-cscf addresses. */
        tmp_ptr = OSAL_strtok(str_ptr, ",");
        i = 0;
        OSAL_memSet(service_ptr->registration.pcscfList, 0, sizeof(service_ptr->registration.pcscfList));
        while ((tmp_ptr != NULL) && (SAPP_MAX_PCSCF_NUM > i)) {
            /*
             * If there is a "sip:" scheme in the outbound proxy then remove it.
             * This is not acceptable to the SIP Stack
             */
            pos_ptr = OSAL_strncasescan(tmp_ptr,
                    ISI_LONG_ADDRESS_STRING_SZ, "sip:");
            if (NULL == pos_ptr) {
                pos_ptr = tmp_ptr;
            }
            else {
                /* Advance off the "sip:" */
                pos_ptr += 4;
            }

            OSAL_snprintf(&service_ptr->registration.pcscfList[i][0],
                    SIP_URI_STRING_MAX_SIZE, "%s", pos_ptr);
            SAPP_dbgPrintf("%s %d: Store pcscfList[%d]: %s\n",
                    __FILE__, __LINE__, i, &service_ptr->registration.pcscfList[i][0]);
            i++;
            tmp_ptr = OSAL_strtok(NULL, ",");
        }

        OSAL_snprintf(service_ptr->sipConfig.config.szOutboundProxy,
                SIP_URI_STRING_MAX_SIZE, "%s", &service_ptr->registration.pcscfList[0][0]);
        pos_ptr = service_ptr->sipConfig.config.szOutboundProxy;
        /* Store proxy address and port for restoring from ipsec */
        SAPP_setHostAndPort(pos_ptr,
                proxyAddrStr, SAPP_STRING_SZ,
                &service_ptr->ipsecObj.defaultProxyPort);

        if (s_ptr->settings.server[0] != 0) {
            /*
             * Then the command is to 'enable' so set the regular proxy
             * to the same value as the the outbound proxy but with a UDP
             * compliant URI scheme.
             */
            OSAL_snprintf(service_ptr->sipConfig.config.szProxy,
                SIP_URI_STRING_MAX_SIZE, "sip:%s", pos_ptr);
        }
        if (service_ptr->sipConfig.uaId != 0) {
            /* Then UA is currently active so call UA_Modify() */
            UA_Modify(
                    service_ptr->sipConfig.uaId,
                    service_ptr->sipConfig.config.szProxy,
                    service_ptr->sipConfig.config.szOutboundProxy, NULL,
                    NULL, NULL, NULL, NULL, NULL, 20, 0);
        }
        break;
    case ISI_SERVER_TYPE_STORAGE:
        /*
         * Set whether or not we are using an xcap server for data storage.  If
         * the xcap root is set then we will do SIMPLE with data storage
         * on a server (using XCAP protocol).  Otherwise, we do presence/IM via
         * endpoint to endpoint.
         */
#ifdef INCLUDE_SIMPLE
        OSAL_snprintf(service_ptr->simple.xcap.root, SIMPL_STRING_SZ, "%s",
                s_ptr->settings.server);

        if (0 == service_ptr->simple.xcap.root[0]) {
            service_ptr->simple.useStorageServer = OSAL_FALSE;
        }
        else {
            service_ptr->simple.useStorageServer = OSAL_TRUE;
        }
#endif
        break;
    case ISI_SERVER_TYPE_CHAT:
        /* Set the server to use for adhoc IM sessions. */
#ifdef INCLUDE_SIMPLE
        OSAL_snprintf(service_ptr->simple.chatServer, SIMPL_STRING_SZ, "%s",
                s_ptr->settings.server);
#endif
        break;
    case ISI_SERVER_TYPE_RELAY:
    default:
        break;
    }
    return;
}

/*
 * ======== _SAPP_sipServicePort() ========
 *
 * This function is called when ISI has commanded SAPP to set a local port and
 * pool size for a specific port type.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServicePort(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    ISIP_Service    *s_ptr)
{

    switch (s_ptr->settings.port.portType) {
        case ISI_PORT_TYPE_SIP:
            service_ptr->defaultPort = s_ptr->settings.port.portNum;
            /* Also set ipsec unprotected default port */
            service_ptr->ipsecObj.defaultPort = service_ptr->defaultPort;
            break;
        case ISI_PORT_TYPE_AUDIO:
            service_ptr->artpInfc.nextInfc.port = s_ptr->settings.port.portNum;
            service_ptr->artpInfc.startPort = s_ptr->settings.port.portNum;
            service_ptr->artpInfc.endPort = s_ptr->settings.port.portNum +
                    s_ptr->settings.port.poolSize;
            break;
        case ISI_PORT_TYPE_VIDEO:
            service_ptr->vrtpInfc.nextInfc.port = s_ptr->settings.port.portNum;
            service_ptr->vrtpInfc.startPort = s_ptr->settings.port.portNum;
            service_ptr->vrtpInfc.endPort = s_ptr->settings.port.portNum +
                    s_ptr->settings.port.poolSize;
            break;
        default:
            break;
    }
    
    return;
}

/*
 * ======== _SAPP_sipServiceIpsec() ========
 *
 * This function is called when ISI has commanded SAPP to set IPSec infomation
 * specified in service_ptr.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceIpsec(
    SAPP_ServiceObj    *service_ptr,
    ISIP_Service       *s_ptr)
{
    /* Set protected start port */
    service_ptr->ipsecObj.startProtectedPort =
            s_ptr->settings.ipsec.cfg.protectedPort;
    /* Set pool size of protected port */
    service_ptr->ipsecObj.pPortPoolSize =
            s_ptr->settings.ipsec.cfg.protectedPortPoolSz;
    /*
     * Set last used protected port to the end of pool, so that it wil start
     * from beginning.
     */
    service_ptr->ipsecObj.lastProtectedPort  =
            s_ptr->settings.ipsec.cfg.protectedPort +
            s_ptr->settings.ipsec.cfg.protectedPortPoolSz;
    /* Set start spi */
    service_ptr->ipsecObj.startSpi = s_ptr->settings.ipsec.cfg.spi;
    /*
     * Set last used spi to the end of pool, so that it wil start from
     * beginning.
     */
    service_ptr->ipsecObj.lastSpi  = s_ptr->settings.ipsec.cfg.spi +
            s_ptr->settings.ipsec.cfg.spiPoolSz;
    /* Set spi pool size */
    service_ptr->ipsecObj.spiPoolSize =
            s_ptr->settings.ipsec.cfg.spiPoolSz;

    return;
}

/*
 * ======== _SAPP_sipServiceEmergency() ========
 *
 * This function is called when ISI has commanded SAPP to set emergency to 
 * service.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceEmergency(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    ISIP_Service    *s_ptr)
{

    service_ptr->isEmergency = s_ptr->settings.isEmergency;
    return;
}

/*
 * ======== _SAPP_sipServiceUri() ========
 *
 * This function is called when ISI has commanded SAPP to set the "URI"
 * value for the "service" specified in service_ptr. It is considered
 * the 'command handler' for ISI commands pertaining to setting the URI.
 * An examples of a URI is "sip:38839@fwd.pulver.com".
 * This is also known as the SIP 'AOR' (Address of Record).
 * This function will also set the FQDN based on the username portion of
 * of the AOR and the 'hostname'.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceUri(
    SAPP_SipObj        *sip_ptr,
    SAPP_ServiceObj    *service_ptr,
    ISIP_Service       *s_ptr)
{
    tUaConfig *config_ptr;
    char      *pos_ptr;
    char      *end_ptr;
    int        length;
    char       hostname[SAPP_STRING_SZ];
    
    config_ptr = &service_ptr->sipConfig.config;

    pos_ptr = s_ptr->settings.uri;
    
    /* Construct the aor for SIP */
    OSAL_snprintf(config_ptr->aor[0].szUri, 
            SIP_URI_STRING_MAX_SIZE, "%s", pos_ptr);

#ifdef INCLUDE_SIMPLE
    /* Set the XCAP username to the same as the url */
    OSAL_snprintf(
            service_ptr->simple.xcap.username,
            SIMPL_STRING_SZ, "%s", pos_ptr);
#endif
    
    /* 
     * Add another AOR just in case users want to use 'privacy'.
     * (caller ID blocking)
     */
    OSAL_snprintf(config_ptr->aor[1].szUri, 
            SIP_URI_STRING_MAX_SIZE, "%s", SAPP_ANONYMOUS_CALL_ID_URI);
    
    /*
     * Construct the FQDN at this point also based on the
     * username portion of the AOR and the hostname.
     */
    
    if (OSAL_netIsAddrIpv6(&service_ptr->sipInfc)) {
        /* ipv6 address need wrap with []*/
        OSAL_snprintf(hostname, SAPP_STRING_SZ, "[%s]", sip_ptr->hostname);
    }
    else {
        OSAL_snprintf(hostname, SAPP_STRING_SZ, "%s", sip_ptr->hostname);
    }

     /* Get the end of the username portion */
    if (NULL == (end_ptr = OSAL_strnscan(pos_ptr,
            ISI_ADDRESS_STRING_SZ, "@"))) {
        /* Then there isn't a domain, so copy the whole thing */
        OSAL_snprintf(config_ptr->szFqdn, SIP_URI_STRING_MAX_SIZE, "%s@%s",
            pos_ptr, hostname);
    }
    else {
        /* 
         * Then the end_ptr is the end of the username.
         */
        length = end_ptr - pos_ptr;
        /* make sure we don't over flow anything */
        length = (SIP_URI_STRING_MAX_SIZE <= length) ? 
            (SIP_URI_STRING_MAX_SIZE - 1) : length;
        OSAL_memCpy(config_ptr->szFqdn, pos_ptr, length);
        end_ptr = config_ptr->szFqdn + length;
        // NULL terminate
        *end_ptr = 0;
        // Now build it. 
         OSAL_snprintf(end_ptr, SIP_URI_STRING_MAX_SIZE - length,
                 "@%s", hostname);
    }
        
#ifdef INCLUDE_SIMPLE
    /* Get the xid, same as the URI/AOR */
    OSAL_snprintf(service_ptr->simple.xcap.xui, SIMPL_STRING_SZ, "%s", pos_ptr);
#endif
    
    
    if (service_ptr->sipConfig.uaId != 0) {
        /* Then UA is currently active so call UA_Modify() */
        UA_Modify(service_ptr->sipConfig.uaId, NULL, 0, NULL, NULL,
                config_ptr->szFqdn, config_ptr->aor, NULL, NULL, 20, 0);
    }
    return;
}

/*
 * ======== _SAPP_sipServiceInstanceId() ========
 *
 * This function is called when ISI has commanded SAPP to set the instance id
 * value for the "service" specified in service_ptr.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceInstanceId(
    SAPP_ServiceObj    *service_ptr,
    ISIP_Service       *s_ptr)
{
    OSAL_snprintf(service_ptr->instanceId, SIP_INSTANCE_STR_SIZE, "%s",
            s_ptr->settings.instanceId);
    return;
}

/*
 * ======== _SAPP_sipServiceBsid() ========
 *
 * This function is called when ISI has commanded SAPP to set the
 * bsid for a wimax service specified in service_ptr.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceBsid(
    SAPP_ServiceObj    *service_ptr,
    ISIP_Service       *s_ptr)
{
    tLocalIpConn *conn_ptr;

    /* Set the bs type for SIP */
    service_ptr->bsId.type = s_ptr->settings.bsId.type;
    /* Set the bs id for SIP */
    OSAL_snprintf(service_ptr->bsId.szBsId, SAPP_BSID_STRING_SZ,
            "%s", s_ptr->settings.bsId.szBsId);

    conn_ptr = &service_ptr->sipConfig.localConn;
    if (0 != service_ptr->bsId.szBsId[0]) {
        OSAL_strncpy(conn_ptr->nwAccess.id, service_ptr->bsId.szBsId,
                sizeof(conn_ptr->nwAccess.id));
        switch (service_ptr->bsId.type) {
            case ISI_NETWORK_ACCESS_TYPE_3GPP_GERAN:
                conn_ptr->nwAccess.type = eNwAccessTypeGeran;
                break;
            case ISI_NETWORK_ACCESS_TYPE_3GPP_UTRAN_FDD:
                conn_ptr->nwAccess.type = eNwAccessTypeUtranFdd;
                break;
            case ISI_NETWORK_ACCESS_TYPE_3GPP_UTRAN_TDD:
                conn_ptr->nwAccess.type = eNwAccessTypeUtranTdd;
                break;
            case ISI_NETWORK_ACCESS_TYPE_3GPP_E_UTRAN_FDD:
                conn_ptr->nwAccess.type = eNwAccessTypeEUtranFdd;
                break;
            case ISI_NETWORK_ACCESS_TYPE_3GPP_E_UTRAN_TDD:
                conn_ptr->nwAccess.type = eNwAccessTypeEUtranTdd;
                break;
            case ISI_NETWORK_ACCESS_TYPE_IEEE_802_11:
                conn_ptr->nwAccess.type = eNwAccessType80211;
                break;
            /* Add more if needed. */
            default: 
                SAPP_dbgPrintf("Unsupported Network Access Type:%d\n",
                        service_ptr->bsId.type);
                break;
        }
    }

    /* Update Access info. */
    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(&service_ptr->sipConfig.localConn.fd)) {
        SIP_updateAccessNwInfo(conn_ptr->fd,
                &conn_ptr->nwAccess);
    }
    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(&service_ptr->sipInfcTcpClientFd)) {
         SIP_updateAccessNwInfo(service_ptr->sipInfcTcpClientFd,
                &conn_ptr->nwAccess);
    }
    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(&service_ptr->sipInfcProtectedClientFd)) {
         SIP_updateAccessNwInfo(service_ptr->sipInfcProtectedClientFd,
                &conn_ptr->nwAccess);
    }
    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(
            &service_ptr->sipInfcProtectedTcpClientFd)) {
         SIP_updateAccessNwInfo(service_ptr->sipInfcProtectedTcpClientFd,
                &conn_ptr->nwAccess);
    }
    return;
}

/*
 * ======== _SAPP_sipServiceImeiUri() ========
 *
 * This function is called when ISI has commanded SAPP to set the
 * imei for a anonymous emergency call specified in service_ptr.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceImeiUri(
    SAPP_ServiceObj    *service_ptr,
    ISIP_Service       *s_ptr)
{

    SAPP_EmgcyObj *emgency_ptr;
    emgency_ptr = &service_ptr->emergencyObj;

    /* Set the imei uri for SIP */
    OSAL_snprintf(
            emgency_ptr->imeiUri,
            SAPP_IMEI_URI_STRING_SZ, "%s",
            s_ptr->settings.imeiUri);

    return;
}

/*
 * ======== _SAPP_sipServiceAuth() ========
 *
 * This function is called when ISI has commanded SAPP to set the
 * authentication credentials for a service specified in service_ptr.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceAuth(
    SAPP_ServiceObj    *service_ptr,
    ISIP_Service       *s_ptr)
{
    tUaConfig *config_ptr;
    config_ptr = &service_ptr->sipConfig.config;

    /* Set the username for SIP */
    OSAL_snprintf(
            config_ptr->authCred[0].szAuthUsername,
            SIP_USERNAME_ARG_STR_SIZE, "%s",
            s_ptr->settings.credentials.username);

#ifdef INCLUDE_SIMPLE
    
    /* Set the same value as the username for XCAP server access */
#if 0
    OSAL_snprintf(
            service_ptr->simple.xcap.username,
            SIMPL_STRING_SZ, "%s",
            s_ptr->settings.credentials.username);
#endif
#endif

    /* Set he realm for SIP */
    OSAL_snprintf(
            config_ptr->authCred[0].szAuthRealm,
            SIP_REALM_ARG_STR_SIZE, "%s",
            s_ptr->settings.credentials.realm);

    /* Set the password for SIP */
    config_ptr->authCred[0].authType = eUA_AUTH_TYPE_PASSWORD;
    OSAL_snprintf(
            config_ptr->authCred[0].u.szAuthPassword,
            SIP_PASSWORD_ARG_STR_SIZE, "%s",
            s_ptr->settings.credentials.password);

#ifdef INCLUDE_SIMPLE
    /* Use the same value to set the password for XCAP */
    OSAL_snprintf(
            service_ptr->simple.xcap.password,
            SIMPL_STRING_SZ, "%s",
            s_ptr->settings.credentials.password);
#endif
    
    if (service_ptr->sipConfig.uaId != 0) {
        /* Then UA is currently active so call UA_Modify() */
        UA_Modify(service_ptr->sipConfig.uaId, NULL, 0, NULL, NULL, NULL, NULL,
                config_ptr->authCred, NULL, 20, 0);
    }
    return;
}

/*
 * ======== _SAPP_getImTransportProto() ========
 * This function is get and test the transport protocol for IM from the
 * configuration.
 *
 * Return:
 *  Nothing.
 */
void _SAPP_getImTransportProto(
    SAPP_SipObj *sip_ptr,
    void        *cfg_ptr)
{
    char *value_ptr;

    /* Get 4g transport protocol of im media */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIMPLE, SETTINGS_TAG_TRANSPORT_PROTO,
            SETTINGS_PARM_PS_MEDIA))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_MSRP_O_TLS,
                sizeof(SETTINGS_PARM_VALUE_MSRP_O_TLS))) {
            sip_ptr->msrpUseTlsFor4g = 1;
        }
        else {
            sip_ptr->msrpUseTlsFor4g = 0;
        }
    }

    /* Get wifi transport protocol im of media */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIMPLE, SETTINGS_TAG_TRANSPORT_PROTO,
            SETTINGS_PARM_WIFI_MEDIA))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_MSRP_O_TLS,
                sizeof(SETTINGS_PARM_VALUE_MSRP_O_TLS))) {
            sip_ptr->msrpUseTlsForWifi = 1;
        }
        else {
            sip_ptr->msrpUseTlsForWifi = 0;
        }
    }
}

/*
 * ======== _SAPP_xmlGetImTransportProto() ========
 * This function is get and test the transport protocol for IM from the
 * xml doc.
 *
 * Return:
 *  Nothing.
 */
void _SAPP_xmlGetImTransportProto(
    SAPP_SipObj *sip_ptr,
    ezxml_t      xml_ptr)
{
    char *value_ptr;

    /* Get 4g transport protocol of im media */
    if (NULL != (value_ptr = SETTINGS_xmlGetParmValue(xml_ptr,
            SETTINGS_TAG_TRANSPORT_PROTO,
            SETTINGS_PARM_PS_MEDIA))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_MSRP_O_TLS,
                sizeof(SETTINGS_PARM_VALUE_MSRP_O_TLS))) {
            sip_ptr->msrpUseTlsFor4g = 1;
        }
        else {
            sip_ptr->msrpUseTlsFor4g = 0;
        }
    }

    /* Get wifi transport protocol im of media */
    if (NULL != (value_ptr = SETTINGS_xmlGetParmValue(xml_ptr,
            SETTINGS_TAG_TRANSPORT_PROTO,
            SETTINGS_PARM_WIFI_MEDIA))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_MSRP_O_TLS,
                sizeof(SETTINGS_PARM_VALUE_MSRP_O_TLS))) {
            sip_ptr->msrpUseTlsForWifi = 1;
        }
        else {
            sip_ptr->msrpUseTlsForWifi = 0;
        }
    }
}

/*
 * ======== _SAPP_getSipTimerSettings() ========
 * This function is get the sip common settings from the configuration
 * and set to service or sip stack.
 *
 * Return:
 *  Nothing.
 */
void _SAPP_getSipTimerSettings(
    void            *cfg_ptr)
{
    char   *value_ptr;
    uint32  t1;
    uint32  t2;
    uint32  t4;
#if defined(PROVIDER_CMCC)
    uint32  tcall;
#endif

    /* Now processing sip timers */
    t1 = t2 = t4 = 0;
    /* Parsing sip timers t1 */
    if (NULL != (value_ptr = SETTINGS_getParmValue(
            SETTINGS_TYPE_SAPP, SETTINGS_NESTED_ONE,
            cfg_ptr, SETTINGS_TAG_PROTOCOL, SETTINGS_TAG_SIP,
            NULL, SETTINGS_PARM_TIMER_T1))) {
        t1 = OSAL_atoi(value_ptr);
    }

    /* Parsing sip timers t2*/
    if (NULL != (value_ptr = SETTINGS_getParmValue(
            SETTINGS_TYPE_SAPP, SETTINGS_NESTED_ONE,
            cfg_ptr, SETTINGS_TAG_PROTOCOL, SETTINGS_TAG_SIP,
            NULL, SETTINGS_PARM_TIMER_T2))) {
        t2 = OSAL_atoi(value_ptr);
    }
    /* Parsing sip timers t1 */
    if (NULL != (value_ptr = SETTINGS_getParmValue(
            SETTINGS_TYPE_SAPP, SETTINGS_NESTED_ONE,
            cfg_ptr, SETTINGS_TAG_PROTOCOL, SETTINGS_TAG_SIP,
            NULL, SETTINGS_PARM_TIMER_T4))) {
        t4 = OSAL_atoi(value_ptr);
    }

    /* Set sip timers if any valid sip timers paramters there */
    if ((0 != t1) || (0 != t2) || (0 != t4)) {
        SIP_setTimers(t1, t2, t4);
    }

#if defined(PROVIDER_CMCC)
    tcall = 0;
    /* Parsing sip timers Tcall */
    if (NULL != (value_ptr = SETTINGS_getParmValue(
            SETTINGS_TYPE_SAPP, SETTINGS_NESTED_ONE,
            cfg_ptr, SETTINGS_TAG_PROTOCOL, SETTINGS_TAG_SIP,
            NULL, SETTINGS_PARM_TIMER_TCALL))) {
        tcall = OSAL_atoi(value_ptr);
    }
    if (0 != tcall) {
        SIP_setTcallTimer(tcall);
    }
#endif

    return;
}

/*
 * ======== _SAPP_xmlGetSipTimerSettings() ========
 * This function is get the sip common settings from the xml doc
 * and set to service or sip stack.
 *
 * Return:
 *  Nothing.
 */
void _SAPP_xmlGetSipTimerSettings(
    ezxml_t          xml_ptr)
{
    char   *value_ptr;
    uint32  t1;
    uint32  t2;
    uint32  t4;
#if defined(PROVIDER_CMCC)
    uint32  tcall;
#endif

    /* Now processing sip timers */
    t1 = t2 = t4 = 0;
    /* Parsing sip timers t1 */
    if (NULL != (value_ptr = SETTINGS_xmlGetParmValue(
            xml_ptr,
            SETTINGS_TAG_SIP,
            SETTINGS_PARM_TIMER_T1))) {
        t1 = OSAL_atoi(value_ptr);
    }

    /* Parsing sip timers t2*/
    if (NULL != (value_ptr = SETTINGS_xmlGetParmValue(
            xml_ptr,
            SETTINGS_TAG_SIP,
            SETTINGS_PARM_TIMER_T2))) {
        t2 = OSAL_atoi(value_ptr);
    }
    /* Parsing sip timers t1 */
    if (NULL != (value_ptr = SETTINGS_xmlGetParmValue(
            xml_ptr,
            SETTINGS_TAG_SIP,
            SETTINGS_PARM_TIMER_T4))) {
        t4 = OSAL_atoi(value_ptr);
    }

    /* Set sip timers if any valid sip timers paramters there */
    if ((0 != t1) || (0 != t2) || (0 != t4)) {
        SIP_setTimers(t1, t2, t4);
    }

#if defined(PROVIDER_CMCC)
    tcall = 0;
    /* Parsing sip timers Tcall */
    if (NULL != (value_ptr = SETTINGS_xmlGetParmValue(
            xml_ptr,
            SETTINGS_TAG_SIP,
            SETTINGS_PARM_TIMER_TCALL))) {
        tcall = OSAL_atoi(value_ptr);
    }
    if (0 != tcall) {
        SIP_setTcallTimer(tcall);
    }
#endif

    return;
}

/*
 * ======== _SAPP_sipServiceSetProvisioningData() ========
 *
 * This function is called when ISI has commanded SAPP to set provisioning data.
 *
 * Sapp supports below paramters configuration:
 *   -Sip timers, T1, T2 and T4.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceSetProvisioningData(
    SAPP_SipObj      *sip_ptr,
    SAPP_ServiceObj  *service_ptr,
    ISIP_Service     *s_ptr)
{
    char   *xmlDoc_ptr;
    int     xmlDocLen;
    ezxml_t xml_ptr;
    ezxml_t child_ptr;
    char   *value_ptr;

    /* Parse the paramter xml doc */
    xmlDoc_ptr = s_ptr->settings.provisioningData;
    xmlDocLen = OSAL_strlen(xmlDoc_ptr);
    xml_ptr = ezxml_parse_str(xmlDoc_ptr, xmlDocLen);

    /* Get and set sip timer settings */
    _SAPP_xmlGetSipTimerSettings(xml_ptr);

    /* Get q value */
    if (NULL != (value_ptr = SETTINGS_xmlGetParmValue(
            xml_ptr,
            SETTINGS_TAG_SIP,
            SETTINGS_PARM_Q_VALUE))) {
        /* Set q value here. */
        _SAPP_sipServiceQValue(service_ptr, value_ptr);
    }

    /* Get keep alive enabled */
    if (NULL != (value_ptr = SETTINGS_xmlGetParmValue(
            xml_ptr,
            SETTINGS_TAG_SIP,
            SETTINGS_PARM_KEEP_ALIVE_ENABLED))) {
        if (1 == OSAL_atoi(value_ptr)) {
            service_ptr->natKeepaliveEnable = OSAL_TRUE;
        }
        else {
            service_ptr->natKeepaliveEnable = OSAL_FALSE;
        }
    }

    /* Get RegRetryBaseTime and RegRetryMaxTime */
    if (NULL != (value_ptr = SETTINGS_xmlGetParmValue(
            xml_ptr,
            SETTINGS_TAG_SIP,
            SETTINGS_PARM_REG_RETRY_BASE_TIME))) {
        /* Set retry base time here. */
        service_ptr->registration.regRetryBaseTime = OSAL_atoi(value_ptr);
    }

    if (NULL != (value_ptr = SETTINGS_xmlGetParmValue(
            xml_ptr,
            SETTINGS_TAG_SIP,
            SETTINGS_PARM_REG_RETRY_MAX_TIME))) {
        /* Set retry max time here. */
        service_ptr->registration.regRetryMaxTime = OSAL_atoi(value_ptr);
    }

    /* Get phone-context */
    if (NULL != (value_ptr = SETTINGS_xmlGetParmValue(
            xml_ptr,
            SETTINGS_TAG_SIP,
            SETTINGS_PARM_PHONE_CONTEXT))) {
        /* Set phone context */
        OSAL_strncpy(service_ptr->phoneContext, value_ptr,
                SAPP_PHONE_CONTEXT_STRING_SZ);
    }

    /* Get transport protocol of im */
    child_ptr = ezxml_child(xml_ptr, SETTINGS_TAG_SERVICE);
    _SAPP_xmlGetImTransportProto(sip_ptr, child_ptr);

    /* Free xml_ptr It came from heap when the file was parsed */
    ezxml_free(xml_ptr);
    return;
}

/*
 * ======== _SAPP_sipServiceIdentity() ========
 *
 * This function is called when ISI has commanded SAPP to set hide
 * caller ID info when placing outbound calls.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceIdentity(
    SAPP_ServiceObj *service_ptr,
    ISIP_Service       *s_ptr)
{
    service_ptr->blockCid = s_ptr->settings.identityHide;
    return;
}

/*
 * ======== _SAPP_sipServiceFilePath() ========
 *
 * This function is called when ISI has commanded SAPP to set hide
 * caller ID info when placing outbound calls.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceFilePath(
    SAPP_ServiceObj *service_ptr,
    ISIP_Service    *s_ptr)
{
#ifdef INCLUDE_SIMPLE
    OSAL_snprintf(service_ptr->simple.filePath, SIMPL_FILE_PATH_MAX_SIZE,
            "%s", s_ptr->settings.file.filePath);

    OSAL_snprintf(service_ptr->simple.filePrepend, SIMPL_STRING_SZ,
            "%s", s_ptr->settings.file.filePrepend);
#endif
    return;
}

/*
 * ======== _SAPP_sipServiceBlockUser() ========
 *
 * This function is called when ISI has commanded SAPP to add
 * a user (URI) to a list of URI's used to track remote user's
 * that we want to deny service to.
 *
 * User's that we deny service to are considered to be "blocked".
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceBlockUser(
    SAPP_ServiceObj *service_ptr,
    ISIP_Service       *s_ptr)
{
    /* If the status is invalid from ISI then we want to block */
    if (s_ptr->status == ISIP_STATUS_TRYING) {
        _SAPP_insertBlockedUser(
                &service_ptr->blockedUsers, s_ptr->settings.uri);
    }
    else {
        SAPP_searchBlockedUser(
                &service_ptr->blockedUsers, s_ptr->settings.uri, 1);
    }
    return;
}

/*
 * ======== _SAPP_sipLoadSipCoderString() ========
 *
 * This function will construct a NULL terminated string containing
 * a list of all coder numbers that are in the array of available
 * coders tucked inside the s_ptr object.
 *
 * The resulting terminated string will delimit coder numbers via a
 * ' ' (space).  For example, "0 8 18" means PCMU, PCMA, G729.
 *
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipLoadSipCoderString(
    SAPP_ServiceObj  *s_ptr,
    vint             *prate_ptr,
    char             *target_ptr)
{
    vint num, len, x;

    len = (SIP_MAX_CODER_STR_SIZE - 1);
    x = 0;
    num = 0;
    while (x < ISI_CODER_NUM && (s_ptr->coders[x].szCoderName[0] != 0)) { 
        num += OSAL_snprintf(&target_ptr[num], (len - num), "%d",
                 s_ptr->coders[x].coderNum);
        target_ptr[num++] = ' ';
        x++;
    }
    if (num != 0) {
        num--;
    }
    /* NULL terminate */
    target_ptr[num] = 0;

    /* Also set SIP's default packet rate. This value actually won't get used,
     * but it should be initialized
     */
    *prate_ptr = SAPP_PRATE_DEFAULT_MS;
    return;
}

/*
 * ======== SAPP_getXmlNestedTagAttr() ========
 *
 * This function retrieves the attribute specified in attr_ptr
 * of the XML tag nested underneath a parent tag.
 *
 * Returns:
 *  A pointer to a string containing the attribute value.
 *  NULL if the attibute could not be found or if there is no value.
 *
 */
char* SAPP_getXmlNestedTagAttr(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childTag_ptr,
    const char *attr_ptr)
{
    ezxml_t   child_ptr;
    /* Get the parent tag.  If its exists then search for the child tag */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, parentTag_ptr))) {
        return (NULL);
    }
    /* Get the child */
    if (NULL == (child_ptr = ezxml_child(child_ptr, childTag_ptr))) {
        return (NULL);
    }
    return (char*)ezxml_attr(child_ptr, attr_ptr);
}

/*
 * ======== SAPP_getXml2NestedTagAttr() ========
 *
 * This function retrieves the attribute specified in attr_ptr
 * of the XML tag nested two tags deep underneath a parent tag.
 *
 * Returns:
 *  A pointer to a string containing the attribute value.
 *  NULL if the attribute could not be found or if there is no value.
 *
 */
char* SAPP_getXml2NestedTagAttr(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childOneTag_ptr,
    const char *childTwoTag_ptr,
    const char *attr_ptr)
{
    ezxml_t   child_ptr;
    /* Get the parent tag.  If its exists then search for the child tag */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, parentTag_ptr))) {
        return (NULL);
    }
    /* Get the child */
    if (NULL == (child_ptr = ezxml_child(child_ptr, childOneTag_ptr))) {
        return (NULL);
    }

    /* Get the second child */
    if (NULL == (child_ptr = ezxml_child(child_ptr, childTwoTag_ptr))) {
        return (NULL);
    }

    return (char*)ezxml_attr(child_ptr, attr_ptr);
}

/*
 * ======== SAPP_getXmlNestedTagText() ========
 *
 * This function retrieves the text of the XML tag nested underneath
 * a parent tag.
 *
 * Returns:
 *  A pointer to a string containing the text to the nested tag or
 *  NULL if the tag could not be found or if there is no text.
 *
 */
char* SAPP_getXmlNestedTagText(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childTag_ptr)
{
    ezxml_t   child_ptr;
    /* Get the parent tag.  If its exists then search for the child tag */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, parentTag_ptr))) {
        return (NULL);
    }
    /* Get the child */
    if (NULL == (child_ptr = ezxml_child(child_ptr, childTag_ptr))) {
        return (NULL);
    }
    return ezxml_txt(child_ptr);
}

char* SAPP_getXml2NestedTagText(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childOneTag_ptr,
    const char *childTwoTag_ptr)
{
    ezxml_t   child_ptr;
    /* Get the parent tag.  If its exists then search for the child tag */
    if (NULL == (child_ptr = ezxml_child(xml_ptr, parentTag_ptr))) {
        return (NULL);
    }
    /* Get the first child */
    if (NULL == (child_ptr = ezxml_child(child_ptr, childOneTag_ptr))) {
        return (NULL);
    }
    /* Get the second child */
    if (NULL == (child_ptr = ezxml_child(child_ptr, childTwoTag_ptr))) {
        return (NULL);
    }
    return ezxml_txt(child_ptr);
}

/*
 * ======== _SAPP_sipServiceCoders() ========
 *
 * This function is called when ISI has commanded SAPP to set
 * the list of available coders to the coders specified in the command
 * ("s_ptr").
 *
 * Since available coders stored in SAPP are stored in a SIP/SDP defined
 * object, then this function will also translate the list of available
 * coders defined in s_ptr to the SIP/SDP format.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceCoders(
    SAPP_ServiceObj *service_ptr,
    ISIP_Service    *s_ptr)
{
    /*
     * Update coder list in service.
     */
    _SAPP_decodeCoderIsi2Sapp(
            s_ptr->settings.coders, ISI_CODER_NUM,
            service_ptr->coders, ISI_CODER_NUM);

    /*
     * The SIP stack needs a string of coders to initialize
     * a UA, note that this string is not used, it just requires
     * it's existence but doesn't use it for anything of relevance.
     * Seriously, SIP really doesn't care about the strings value.
     * In future SIP releases this string will be removed from the
     * API.
     */
    _SAPP_sipLoadSipCoderString(service_ptr,
            &service_ptr->sipConfig.config.packetRate,
            service_ptr->sipConfig.config.szCoders);

    if (service_ptr->sipConfig.uaId != 0) {
        /* Then UA is currently active so call UA_Modify() */
        UA_Modify(service_ptr->sipConfig.uaId,
                NULL, 0, NULL, NULL, NULL, NULL, NULL,
                service_ptr->sipConfig.config.szCoders,
                service_ptr->sipConfig.config.packetRate, 0);
    }


    /*
     * Now load the coders.  They will first need to be translated from ISI
     * format to SIP format
     */

    /*
     * audio
     */
    service_ptr->mnsService.sipConfig.amedia.mediaType = eSdpMediaAudio;
    _SAPP_encodeCoderSapp2Sip(service_ptr->coders,
             &service_ptr->mnsService.sipConfig.amedia, eNwAddrNonSpecified);
    /*
     * video
     */
    service_ptr->mnsService.sipConfig.vmedia.mediaType = eSdpMediaVideo;
    _SAPP_encodeCoderSapp2Sip(service_ptr->coders,
            &service_ptr->mnsService.sipConfig.vmedia, eNwAddrNonSpecified);

    return;
}

/*
 * ======== _SAPP_sipServiceActivate() ========
 *
 * This function is called when ISI has commanded SAPP to "active"
 * the service specified in service_ptr.  The function will then call
 * the appropriate SIP interface calls to activate the service.
 * If there are any errors that occur when attempting to "activate"
 * the service, then this function will write error related events
 * to a buffer destined for the ISI module.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceActivate(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    SAPP_Event       *evt_ptr)
{
    tSipHandle ua;

    /* Create the UA if it's not already created */
    if (0 == service_ptr->sipConfig.uaId) {
        /* Then the UA has not been created yet */
        OSAL_snprintf(service_ptr->sipConfig.config.szOutboundProxy,
                SIP_URI_STRING_MAX_SIZE, "%s",
                &service_ptr->registration.pcscfList[
                service_ptr->registration.pcscfIndex][0]);
        if ((ua = UA_Create(&service_ptr->sipConfig.config)) == 0) {
            /*
             * Could not create the UA, so indicate that
             * activating the service failed
             */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);
            return;
        }
        service_ptr->sipConfig.uaId = ua;
    }

    /* Prepare sip transport */
    if (SAPP_OK != SAPP_sipServiceTransportInit(service_ptr, sip_ptr)) {
        /* Tell ISI */
        OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                ISI_EVENT_DESC_STRING_SZ, "REG FAILED: CODE:%d REASON:%s",
                ISI_IMS_XPORT_INIT_FAILURE, ISI_IMS_XPORT_INIT_FAILURE_STR);
        SAPP_serviceIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                ISIP_SERVICE_REASON_NET, ISIP_STATUS_FAILED,
                &evt_ptr->isiMsg);
        SAPP_sendEvent(evt_ptr);

        /* Send another event up for deactivate */
        SAPP_serviceIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID, 
                &evt_ptr->isiMsg);
        /*
         * SAPP_regStart trigger the reg state changed to OFF state so that
         * when retry timer fired, it can re-init transport and register again.
         */
        SAPP_regStart(&service_ptr->registration, service_ptr, evt_ptr, sip_ptr);
        /* Advance P-CSCF and start a retry timer. */
        _SAPP_regAdvancePcscf(service_ptr, sip_ptr);
        _SAPP_regStartRetryTmr(service_ptr, NULL);
        return;
    }

    /* Otherwise the user has explicitly set the registrar, try to register */
    service_ptr->registration.regFailCount = 0;
    service_ptr->registration.pcscfIndex = 0;
    SAPP_regStart(&service_ptr->registration, service_ptr, evt_ptr, sip_ptr);
    return;
}

/*
 * ======== _SAPP_sipServiceDeactivate() ========
 *
 * This function is called when ISI has commanded SAPP to "deactivate"
 * the service specified in service_ptr.  The function will then call
 * the appropriate SIP interface calls to deactivate (and de-register)
 * the service.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceDeactivate(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr,
    SAPP_Event      *evt_ptr)
{
    /* Shutdown any presence related subscriptions and halt any publications */
#ifdef INCLUDE_SIMPLE
    SIMPL_shutdown(&service_ptr->simple, service_ptr, evt_ptr);
#endif

    /*
     * De-register this endpoint.  Even if we did not previously
     * register we can still call this function.  SIP will just return if
     * there's nothing to do
     */
    SAPP_regStop(&service_ptr->registration, service_ptr, evt_ptr, sip_ptr);

    return;
}

/*
 * ======== _SAPP_sipServiceAkaAuthResp() ========
 *
 * This function is called when ISI has commanded SAPP to set AKA
 * authentication response. The function will then call the appropriate
 * SIP interface calls to register again with AKA authentication response.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipServiceAkaAuthResp(
    SAPP_ServiceObj *service_ptr,
    SAPP_SipObj     *sip_ptr,
    ISIP_Service    *s_ptr,
    SAPP_Event      *evt_ptr)
{
    /* Set result, resp, auts, ck and ik */
    service_ptr->akaAuthResult = s_ptr->settings.akaAuthResp.result;
    OSAL_memCpy(service_ptr->akaAuthResp, s_ptr->settings.akaAuthResp.resp,
            SAPP_AKA_AUTH_RESP_SZ);
    OSAL_memCpy(service_ptr->akaAuthAuts, s_ptr->settings.akaAuthResp.auts,
            SAPP_AKA_AUTH_AUTS_SZ);
    OSAL_memCpy(service_ptr->akaAuthCk, s_ptr->settings.akaAuthResp.ck,
            SAPP_AKA_AUTH_CK_SZ);
    OSAL_memCpy(service_ptr->akaAuthIk, s_ptr->settings.akaAuthResp.ik,
            SAPP_AKA_AUTH_IK_SZ);
    service_ptr->akaAuthResLength = s_ptr->settings.akaAuthResp.resLength;

    service_ptr->akaAuthRespSet = 1;

    SAPP_dbgPrintf("_SAPP_sipServiceAkaAuthResp. result: %d",
            service_ptr->akaAuthResult);

    /* Check if it's requried to set IPSec SA. */
    if (service_ptr->sipConfig.useIpSec) {
        /*
         * Don't set key if the response is empty string,
         * i.e isim autenticate failed
         */
        if ('\0' != service_ptr->akaAuthResp[0]) {
            /*
             * Set protected port.
             * If will also set ipsec keys and create ipsec SAs.
             */
            if (SAPP_OK != _SAPP_ipsecSetProtectedPort(service_ptr, sip_ptr)) {
                SAPP_dbgPrintf("%s %d: Update protected port failed.\n",
                            __FILE__, __LINE__);
                /*
                 * Change back to default unprotected port and
                 * do not need to send register.
                 */
                _SAPP_ipsecSetDefaultPort(service_ptr, sip_ptr);
                /* Stop register */
                SAPP_regStop(&service_ptr->registration, service_ptr, evt_ptr, sip_ptr);
                return;
            }
        }
    }

    /* Register with AKA authentication response */
    SAPP_regReReg(&service_ptr->registration, service_ptr, evt_ptr, sip_ptr);
    return;
}

/*
 * ======== SAPP_sendEvent() ========
 *
 * This function sends event from GAPP to ISI.
 *
 * Returns:
 * SAPP_OK  : Event sent.
 * SAPP_ERR : Error.
 */
vint SAPP_sendEvent(
    SAPP_Event *evt_ptr)
{
    if (OSAL_SUCCESS != OSAL_msgQSend(evt_ptr->isiEvt, (char *)&evt_ptr->isiMsg,
            sizeof(ISIP_Message), OSAL_WAIT_FOREVER, NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED qId=%p\n", __FUNCTION__,
                __LINE__, evt_ptr->isiEvt);
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

/*
 * ======== _SAPP_sipService() ========
 *
 * This function is the entry point for commands that come from ISI
 * that pertain to "service" settings.  It's the first place a service
 * command begins to be processed.
 *
 * cmd_ptr : A pointer to the command block that came from ISI. All command
 *           details are inside here.
 *
 * sip_ptr : A pointer to an object used internally in SAPP to manage services
 *
 * isi_ptr : A pointer to a buffer area that is used to write ISI events to.
 *           Events that are written to this buffer are ultimately sent up to
 *           the ISI layer.
 *
 * Returns:
 *   Nothing.
 */
void SAPP_isiServiceCmd(
    ISIP_Message    *cmd_ptr,
    SAPP_SipObj     *sip_ptr,
    SAPP_Event      *evt_ptr)
{
    SAPP_ServiceObj *service_ptr;
    ISIP_Service       *s_ptr;

    s_ptr = &cmd_ptr->msg.service;

    /* For 'new' services we look for an available internal service object */
    if (s_ptr->reason == ISIP_SERVICE_REASON_CREATE) {
        /* 
         * For YES phone.  YTL might remove in future.  See if it
         * already exists.  This can happen if ISI suddenly died and
         * restarted.  So this works like a 'recreate' command.
         */
        service_ptr = SAPP_findServiceViaServiceId(sip_ptr, cmd_ptr->id);
        if (service_ptr != NULL) {
            _SAPP_sipServiceDestroy(service_ptr);
        }
        else {
            service_ptr = SAPP_findServiceViaServiceId(sip_ptr, 0);
            if (service_ptr == NULL) {
                /*
                 * Then there is no available service resource.
                 * Send ISI an error.
                 */
                SAPP_serviceIsiEvt(cmd_ptr->id, service_ptr->protocolId,
                        ISIP_SERVICE_REASON_CREATE,
                        ISIP_STATUS_FAILED, &evt_ptr->isiMsg);
            }
        }
    }
    else {
        /* It's for an existing service, so search for it via the service Id */
        service_ptr = SAPP_findServiceViaServiceId(sip_ptr, cmd_ptr->id);
    }

    if (service_ptr == NULL) {
        /* Then nothing to perform so quietly ignore the command */
        return;
    }

    /* Now 'switch' on the reason to perform the right operation */
    switch (cmd_ptr->msg.service.reason) {
    case ISIP_SERVICE_REASON_CREATE:
        /* Initialize the SAPP object for sip */
        _SAPP_sipServiceInit(sip_ptr, service_ptr, cmd_ptr->id, s_ptr,
                cmd_ptr->protocol);
        break;
    case ISIP_SERVICE_REASON_DESTROY:
        _SAPP_sipServiceDestroy(service_ptr);
        break;
    case ISIP_SERVICE_REASON_SERVER:
        _SAPP_sipServiceServer(sip_ptr, service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_URI:
        _SAPP_sipServiceUri(sip_ptr, service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_INSTANCEID:
        _SAPP_sipServiceInstanceId(service_ptr, s_ptr);
        break;      
    case ISIP_SERVICE_REASON_BSID:
        _SAPP_sipServiceBsid(service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_AUTH:
        _SAPP_sipServiceAuth(service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_IDENTITY:
        _SAPP_sipServiceIdentity(service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_CODERS:
        _SAPP_sipServiceCoders(service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_ACTIVATE:
        _SAPP_sipServiceActivate(service_ptr, sip_ptr, evt_ptr);
        break;
    case ISIP_SERVICE_REASON_DEACTIVATE:
        _SAPP_sipServiceDeactivate(service_ptr, sip_ptr, evt_ptr);
        break;
    case ISIP_SERVICE_REASON_BLOCKUSER:
        _SAPP_sipServiceBlockUser(service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_FILE:
        _SAPP_sipServiceFilePath(service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_AUTH_AKA_RESPONSE:
        _SAPP_sipServiceAkaAuthResp(service_ptr, sip_ptr, s_ptr, evt_ptr);
        break;
    case ISIP_SERVICE_REASON_CAPABILITIES:
        _SAPP_isiCapabilitiesSetCmd(service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_PORT:
        _SAPP_sipServicePort(sip_ptr, service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_EMERGENCY:
        _SAPP_sipServiceEmergency(sip_ptr, service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_IMEI_URI:
        _SAPP_sipServiceImeiUri(service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_IPSEC:
        _SAPP_sipServiceIpsec(service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_SET_PROVISIONING_DATA:
        _SAPP_sipServiceSetProvisioningData(sip_ptr, service_ptr, s_ptr);
        break;
    case ISIP_SERVICE_REASON_NET:
        /* In the command is the information about the
        * network interface to use...
        * cmd_ptr->msg.service.settings.interface.name =
        *      The Name of the interface.
        * cmd_ptr->msg.service.settings.interface.address =
        *      The Address of the interface.
        *
        * We must now update the SAPP module with the new interface info.
        *      This includes:
        * 1) Update the SAPP_SipObj.
        * 2) Updating the SIP stack.
        */
        SAPP_radioInterfaceUpdate(sip_ptr, service_ptr, evt_ptr,
                cmd_ptr->msg.service.settings.interface.address,
                cmd_ptr->msg.service.settings.interface.name);
        break;
    case ISIP_SERVICE_REASON_HANDOFF:
    default:
        break;
    } /* End of switch */
    return;
}

/*
 * ======== SAPP_sipAddPreconfiguredRoute() ========
 * This function will write a "Route" header field to the target buffer if there
 * is a "Preconfigured Route" set.  Preconfigured routes are typically set by
 * the registrar returning a "Service-Route" header field.
 *
 * Returns:
 *   SAPP_OK  : A "Route" header field was written to the target.
 *   SAPP_ERR : No Preconfigured route exists.  Nothing was written to the
 *              target.
 */
vint SAPP_sipAddPreconfiguredRoute(
    SAPP_RegObj   *reg_ptr,
    char          *target_ptr,
    vint           targetLen)

{
    if (0 != reg_ptr->preconfiguredRoute[0]) {
        /* Then there is a preconfigured route */
        OSAL_snprintf(target_ptr, targetLen, "%s %s", SAPP_ROUTE_HF,
                reg_ptr->preconfiguredRoute);
        return (SAPP_OK);
    }
    return (SAPP_ERR);
}

/*
 * ======== SAPP_sipAddAuthorization() ========
 * This function will write a "Authorization" header field to the target buffer.
 *
 * Returns:
 *   SAPP_OK  : A "Authorization" header field was written to the target.
 *   SAPP_ERR : Nothing was written to the target.
 */
vint SAPP_sipAddAuthorization(
    SAPP_ServiceObj *service_ptr,
    char            *target_ptr,
    vint             targetLen)

{
    char      *at_ptr;
    char       domain[SAPP_STRING_SZ];
    tUaConfig *cfg_ptr;

    cfg_ptr = &service_ptr->sipConfig.config;

    /* Parse domain from aor[0] */
    if (NULL != (at_ptr = OSAL_strnscan(cfg_ptr->aor[0].szUri,
            SIP_URI_STRING_MAX_SIZE, "@"))) {
        OSAL_snprintf(domain, SAPP_STRING_SZ, "%s", at_ptr + 1);
    }
    else {
        /* There is no domain, leave it empty */
         domain[0] = 0;
    }

    OSAL_snprintf(target_ptr, targetLen,
            "Authorization: Digest username=\"%s\", realm=\"%s\", "
            "uri=\"sip:%s\", nonce=\"\", response=\"\"",
            cfg_ptr->authCred[0].szAuthUsername,
            cfg_ptr->authCred[0].szAuthRealm,
            domain);

    return (SAPP_OK);
}

/*
 * ======== _SAPP_sipDecodeNonce() ========
 * This function is to decode nonce string to RAND and AUTN
 *
 * nonce_ptr : NULL terminated nonce string
 * rand_ptr   : Pointer to rand string
 * randLen    : Length of rand string
 * autn_ptr   : pointer to autn string
 * autnLen    : Length of autn string
 *
 * Returns:
 *  SAPP_OK : Decode nonce successfully
 *  SAPP_ERR: Failed to decode nonce.
 */
vint _SAPP_sipDecodeNonce(
    char  *nonce_ptr,
    uint8 *rand_ptr,
    vint   randLen,
    uint8 *autn_ptr,
    vint   autnLen)
{
    vint    len;
    uint8   nonce[SIP_NONCE_ARG_STR_SIZE + 1];

    /* Zero the nonce */
    OSAL_memSet(nonce, 0, sizeof(nonce));

    if (0 == (len = b64decode(nonce_ptr, OSAL_strlen(nonce_ptr), (char *)nonce))) {
        /* Failed to decode the nonce */
        return (SAPP_ERR);
    }

    /* Verify the length of the decoded nonce */
    if (len < SIP_AUTH_AKA_RANDLEN + SIP_AUTH_AKA_AUTNLEN) {
        return (SAPP_ERR);
    }

    if ((SIP_AUTH_AKA_RANDLEN != randLen) ||
            (SIP_AUTH_AKA_AUTNLEN != autnLen)) {
        return (SAPP_ERR);
    }

    /* Get RAND and AUTN */
    OSAL_memCpy(rand_ptr, nonce, SIP_AUTH_AKA_RANDLEN);
    OSAL_memCpy(autn_ptr, nonce + SIP_AUTH_AKA_RANDLEN, SIP_AUTH_AKA_AUTNLEN);

    SAPP_dbgPrintf("%s %d: rand:%s autn:%s\n",
                    __FILE__, __LINE__, rand_ptr, autn_ptr);

    return (SAPP_OK);
}

/*
 * ======== _SAPP_sipTcpTransportInit() ========
 *
 * This function used to initialize SIP TCP transport.
 *
 * Returns:
 *   SAPP_ERR : Transport failed to initialize.
 *   SAPP_OK : Transport initialized successfully.
 *
 */
vint _SAPP_sipTcpTransportInit(
    SAPP_ServiceObj  *service_ptr,
    OSAL_NetAddress *addr_ptr)
{
    OSAL_NetSockId  *tcpServerfd_ptr;
    OSAL_NetSockId  *tcpClientfd_ptr;
    tLocalIpConn     conn;

    if ((OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(&service_ptr->sipInfcTcpServerFd)) &&
            (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(&service_ptr->sipInfcTcpClientFd))) {
        /* TCP server and client have been init, return OK. */
        return (SAPP_OK);
    }


    if (OSAL_TRUE == OSAL_netIsAddrIpv6(addr_ptr)) {
        addr_ptr->type = OSAL_NET_SOCK_TCP_V6;
    }
    else {
        addr_ptr->type = OSAL_NET_SOCK_TCP;
    }

    /*
     * Create a TCP server socket on the same port as udp port for handling
     * proxy's TCP request if message size larger than mtu.
     */
    tcpServerfd_ptr = &service_ptr->sipInfcTcpServerFd;
    if (OSAL_SUCCESS != IMS_NET_SOCKET(tcpServerfd_ptr, addr_ptr->type)) {
        SAPP_dbgPrintf("%s %d: Create socket failed. fd:%d port:%d\n",
                __FUNCTION__, __LINE__,
                *tcpServerfd_ptr, OSAL_netNtohs(addr_ptr->port));
        return (SAPP_ERR);
    } 
    if (OSAL_SUCCESS != IMS_NET_BIND_SOCKET(tcpServerfd_ptr, addr_ptr)) {
        SAPP_dbgPrintf("%s %d: Bind socket failed. fd:%d port:%d\n",
                __FUNCTION__, __LINE__,
                *tcpServerfd_ptr, OSAL_netNtohs(addr_ptr->port));
        IMS_NET_CLOSE_SOCKET(tcpServerfd_ptr);
        return (SAPP_ERR);
    }
    IMS_NET_GET_SOCKET_ADDRESS(tcpServerfd_ptr, addr_ptr);
    SAPP_dbgPrintf("%s %d: Bind TCP server socket done. fd:%d port:%d\n",
            __FUNCTION__, __LINE__,
            *tcpServerfd_ptr, OSAL_netNtohs(addr_ptr->port));
    if (OSAL_SUCCESS != IMS_NET_LISTEN_ON_SOCKET(tcpServerfd_ptr)) {
        SAPP_dbgPrintf("%s %d: Listen on socket failed. fd:%d port:%d\n",
                __FUNCTION__, __LINE__,
                *tcpServerfd_ptr, OSAL_netNtohs(addr_ptr->port));
        IMS_NET_CLOSE_SOCKET(tcpServerfd_ptr);
        return (SAPP_ERR);
    }

    /* Convert to host byte order */
    OSAL_netAddrPortNtoh(addr_ptr, addr_ptr);
    SIP_replaceServerSocket(*tcpServerfd_ptr, addr_ptr, eTransportTcp,
            &service_ptr->sipConfig.localConn.nwAccess);

    /* Create a TCP socket for sending */
    OSAL_netAddrPortHton(addr_ptr, addr_ptr);
    addr_ptr->port = 0;
    tcpClientfd_ptr = &service_ptr->sipInfcTcpClientFd;
    if (OSAL_SUCCESS != IMS_NET_SOCKET(tcpClientfd_ptr, addr_ptr->type)) {
        SAPP_dbgPrintf("%s %d: Create socket failed. fd:%d port:%d\n",
                __FUNCTION__, __LINE__,
                *tcpClientfd_ptr, OSAL_netNtohs(addr_ptr->port));
        IMS_NET_CLOSE_SOCKET(tcpServerfd_ptr);
        return (SAPP_ERR);
    }
    if (OSAL_SUCCESS != IMS_NET_BIND_SOCKET(tcpClientfd_ptr, addr_ptr)) {
        SAPP_dbgPrintf("%s %d: Bind socket failed. fd:%d port:%d\n",
                __FUNCTION__, __LINE__,
                *tcpClientfd_ptr, OSAL_netNtohs(addr_ptr->port));
        IMS_NET_CLOSE_SOCKET(tcpServerfd_ptr);
        IMS_NET_CLOSE_SOCKET(tcpClientfd_ptr);
        return (SAPP_ERR);
    }

    IMS_NET_GET_SOCKET_ADDRESS(tcpClientfd_ptr, addr_ptr);
    /* Convert to host byte order */
    OSAL_netAddrPortNtoh(&conn.addr, addr_ptr);
    conn.fd = *tcpClientfd_ptr;

    SAPP_dbgPrintf("%s %d: Bind TCP client socket done. fd:%d port:%d\n",
            __FUNCTION__, __LINE__,
            *tcpClientfd_ptr, OSAL_netNtohs(addr_ptr->port));

    if (SIP_FAILED == SIP_clientConnect(
            service_ptr->sipConfig.uaId, &conn, eTransportTcp)) {
        IMS_NET_CLOSE_SOCKET(tcpServerfd_ptr);
        IMS_NET_CLOSE_SOCKET(tcpClientfd_ptr);
        SAPP_dbgPrintf("%s %d: Connect failed. fd:%d\n",
                __FILE__, __LINE__, conn.fd);
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}


/*
 * ========_SAPP_sipServerTimeOut() ========
 *
 * This function is used when getting 504 Server Time out.
 * From 3GPP TS 24.229 5.1.2A.1.6 Abnormal cases, need to check
 * if PAI is the same with service-route/path, and re-register to
 * new PCSCF right away.
 *
 * return: SAPP_OK
 *         SAPP_ERR
 */
vint _SAPP_sipServerTimeOut(
    SAPP_ServiceObj *service_ptr,
    char           **reason_ptr,
    tUaAppEvent     *uaEvt_ptr,
    SAPP_SipObj     *sip_ptr)
{
    char                *value_ptr;
    SAPP_3gppImsAction   xmlAction;
    
    /* Parse the PAI header and message body (XML doc) */
    if (NULL != (value_ptr =
            SAPP_parsePaiHfValue(NULL, uaEvt_ptr))) {
        /* See if PAI is the same with service-route, SAPP_parsePaiHfValue()
         * would remove '<' and '>', so use strscan here.
         */
        if (NULL == OSAL_strscan(
                service_ptr->registration.preconfiguredRoute,
                value_ptr)) {
            /* See if PAI is the same with Path. */
            if (0 != OSAL_strncmp(service_ptr->lastPathUri, value_ptr,
                    OSAL_strlen(value_ptr))) {
                return (SAPP_ERR);
            }
        }

        /* Decode xml doc */
        if (SAPP_OK != _SAPP_xmlDecode3gppImsDoc(
                uaEvt_ptr->msgBody.payLoad.data,
                uaEvt_ptr->msgBody.payLoad.length,
                reason_ptr,
                &xmlAction)) {
            /* Decode failed */
            return (SAPP_ERR);
        }
        /* Need action: initial-registration. */
        if (SAPP_XML_ACTION_INITIAL_REG == xmlAction) {
            /* re-Register. */
            _SAPP_regAdvancePcscf(service_ptr, sip_ptr);
            /* Run state machine to reset. */
            SAPP_regReset(&service_ptr->registration, service_ptr,
                    &sip_ptr->event, sip_ptr);
            SAPP_regRestart(&service_ptr->registration, service_ptr,
                    &sip_ptr->event, sip_ptr);
            return (SAPP_OK);
        }
        return (SAPP_ERR);
    }
    return (SAPP_ERR);
}

/*
 * ======== SAPP_sipServiceTransportInit() ========
 *
 * This function used to initialize SIP service transport.
 *
 * Returns:
 *   SAPP_ERR : Transport failed to initialize.
 *   SAPP_OK : Transport initialized successfully.
 *
 */
vint SAPP_sipServiceTransportInit(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr)
{
    OSAL_NetAddress *addr_ptr;
    OSAL_NetAddress  addr;
    OSAL_NetSockId  *fd_ptr;
    SAPP_RegObj     *reg_ptr;

    /* Get transport type */
    service_ptr->sipTransportType = SIP_getTransportType(service_ptr->sipConfig.uaId);
    service_ptr->sipInfc.port = service_ptr->defaultPort;

    /* Clean previous transport stuff*/
    SAPP_sipServiceTransportClean(service_ptr, sip_ptr);

    if (service_ptr->sipConfig.useIpSec) {
        _SAPP_ipsecInit(service_ptr, sip_ptr);
    }

    addr_ptr = &service_ptr->sipInfc;

    if (eTransportUdp == service_ptr->sipTransportType) {
        if (OSAL_netIsAddrIpv6(addr_ptr)) {
            addr_ptr->type = OSAL_NET_SOCK_UDP_V6;
        }
        else {
            addr_ptr->type = OSAL_NET_SOCK_UDP;
        }
    }
    else if (eTransportTcp == service_ptr->sipTransportType) {
        /* Always use random port for tcp except ipsec */
        addr_ptr->port = 0;
        if (OSAL_netIsAddrIpv6(addr_ptr)) {
            addr_ptr->type = OSAL_NET_SOCK_TCP_V6;
        }
        else {
            addr_ptr->type = OSAL_NET_SOCK_TCP;
        }
    }
    else if (eTransportTls == service_ptr->sipTransportType) {
        /* Always use random port for tls/tcp except ipsec */
        addr_ptr->port = 0;
        if (OSAL_netIsAddrIpv6(addr_ptr)) {
            addr_ptr->type = OSAL_NET_SOCK_TCP_V6;
        }
        else {
            addr_ptr->type = OSAL_NET_SOCK_TCP;
        }
    }
    else {
        /* Other transport type */
        SAPP_dbgPrintf("%s %d: Unsupported tranport type %d.\n",
                __FILE__, __LINE__, service_ptr->sipTransportType);
        /* Clean transport */ 
        SAPP_sipServiceTransportClean(service_ptr, sip_ptr);
        return (SAPP_ERR);
    }

    fd_ptr = &service_ptr->sipInfcFd;

    if (OSAL_SUCCESS != OSAL_netIsSocketIdValid(fd_ptr)) {
        /* Convert to network byte order */
        OSAL_netAddrPortHton(&addr, &service_ptr->sipInfc);

        /* Create a socket for the SIP network interface */
        if (OSAL_SUCCESS != IMS_NET_SOCKET(fd_ptr, addr.type)) {
            SAPP_dbgPrintf("%s %d: Socket create failed.\n",
                    __FILE__, __LINE__);
            /* Clean transport */ 
            SAPP_sipServiceTransportClean(service_ptr, sip_ptr);
            return (SAPP_ERR);
        }

        /* Bind the port */
        if (OSAL_SUCCESS != IMS_NET_BIND_SOCKET(fd_ptr, &addr)) {
            SAPP_dbgPrintf("%s %d: Bind socket failed. addr:%x,"
                    "port:%x(network byte order).\n",
                    __FILE__, __LINE__, addr.ipv4, addr.port);
            /* Clean transport */ 
            SAPP_sipServiceTransportClean(service_ptr, sip_ptr);
            return (SAPP_ERR);
        }

        SAPP_dbgPrintf("%s %d: Bind client socket done. fd:%d port:%d\n",
                __FILE__, __LINE__, *fd_ptr, OSAL_netNtohs(addr.port));
    }
    else {
        SAPP_dbgPrintf("%s %d: Reuse existing socket fd:%d port:%d\n",
                __FILE__, __LINE__, *fd_ptr, service_ptr->sipInfc.port);
    }

    if (eTransportUdp == service_ptr->sipTransportType) {
        /* Get the port we bound */
        IMS_NET_GET_SOCKET_ADDRESS(fd_ptr, &addr);
        /* Convert to host byte order */
        OSAL_netAddrPortNtoh(&service_ptr->sipConfig.localConn.addr, &addr);

        /*
         * If IPSec is disabled and MTU is set to a value,
         * to init TCP transport for sending SIP request over TCP when
         * message length is larger then MTU size.
         */
        if ((0 == service_ptr->sipConfig.useIpSec) &&
                sip_ptr->mtu &&
                ((OSAL_NET_SOCK_UDP == addr.type) ||
                (OSAL_NET_SOCK_UDP_V6 == addr.type))) {
            if (SAPP_ERR == _SAPP_sipTcpTransportInit(service_ptr, &addr)) {
                /* Clean transport */ 
                SAPP_sipServiceTransportClean(service_ptr, sip_ptr);
                SAPP_dbgPrintf("%s %d: Init TCP Transport FAILED.\n",
                        __FILE__, __LINE__);
                return (SAPP_ERR);
            }
        }
    }

    service_ptr->sipConfig.localConn.fd = service_ptr->sipInfcFd;

    /* Set local connection in regObj */
    reg_ptr = &service_ptr->registration;
    reg_ptr->lclConn = service_ptr->sipConfig.localConn;

    /* Make connection for ASAPP so that it can receive packet from VPR */
    if (SIP_FAILED == SR_clientConnect(service_ptr->sipConfig.uaId,
            &service_ptr->sipConfig.localConn, eTransportNone)) {
        SAPP_dbgPrintf("%s %d: Connect failed. fd:%d port:%d\n",
                __FILE__, __LINE__, *fd_ptr, service_ptr->sipInfc.port);
        SAPP_sipServiceTransportClean(service_ptr, sip_ptr);
        return (SAPP_ERR);
    }

    service_ptr->isTransportReady = OSAL_TRUE;

    return (SAPP_OK);
}

/*
 * ======== _SAPP_sipServiceIsTransportReady() ========
 *
 * This function used to check if the sip transport is initialized.
 *
 * Returns:
 *   OSAL_TRUE: Sip transport is initialized.
 *   OSAL_FALSE: Otherwise.
 */
vint _SAPP_sipServiceIsTransportReady(
    SAPP_ServiceObj  *service_ptr)
{
    return (service_ptr->isTransportReady);
}


/*
 * ======== SAPP_sipServiceTransportClean() ========
 *
 * This function used to close SIP service transport.
 *
 * Returns:
 *   SAPP_ERR : Transport failed to clean.
 *   SAPP_OK : Transport cleaned successfully.
 *
 */
vint SAPP_sipServiceTransportClean(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr)
{
    OSAL_NetAddress addr;

    if (service_ptr->sipConfig.useIpSec) {
        _SAPP_ipsecClean(service_ptr, sip_ptr);
    }

    /* Get the socket type and port number */
    IMS_NET_GET_SOCKET_ADDRESS(&service_ptr->sipInfcFd, &addr);
    OSAL_netAddrPortNtoh(&addr, &addr);

    /*
     * We will reuse the socket if it's UDP with specified port
     * So don't close socket if we want to reuse it.
     */
    if ((eTransportTcp == service_ptr->sipTransportType) ||
            ((eTransportUdp == service_ptr->sipTransportType) &&
            !OSAL_netIsAddrPortEqual(&addr, 
            &service_ptr->sipConfig.localConn.addr))) {
        if (OSAL_SUCCESS == OSAL_netIsSocketIdValid(&service_ptr->sipInfcFd)) {
            SAPP_dbgPrintf("%s %d: SAPP_sipTransportClean. Close socket. "
                    "fd:%d\n", __FILE__, __LINE__, service_ptr->sipInfcFd);
            SIP_CloseConnection(service_ptr->sipInfcFd);
            IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcFd);
            service_ptr->sipInfcFd = OSAL_NET_SOCK_INVALID_ID;
        }

        if (OSAL_SUCCESS ==
                OSAL_netIsSocketIdValid(&service_ptr->sipInfcTcpServerFd)) {
            SAPP_dbgPrintf("%s %d: SAPP_sipTransportClean. Close socket. "
                    "fd:%d\n", __FILE__, __LINE__,
                    service_ptr->sipInfcTcpServerFd);
            SIP_CloseConnection(service_ptr->sipInfcTcpServerFd);
            IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcTcpServerFd);
            service_ptr->sipInfcTcpServerFd = OSAL_NET_SOCK_INVALID_ID;
        }
        if (OSAL_SUCCESS ==
                OSAL_netIsSocketIdValid(&service_ptr->sipInfcTcpClientFd)) {
            SAPP_dbgPrintf("%s %d: SAPP_sipTransportClean. Close socket. "
                    "fd:%d\n", __FILE__, __LINE__,
                    service_ptr->sipInfcTcpClientFd);
            SIP_CloseConnection(service_ptr->sipInfcTcpClientFd);
            IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcTcpClientFd);
            service_ptr->sipInfcTcpClientFd = OSAL_NET_SOCK_INVALID_ID;
        }
    }
    /* Set flag as OSAL_FALSE */
    service_ptr->isTransportReady = OSAL_FALSE;


    return (SAPP_OK);
}

/*
 * ======== SAPP_sipServiceTransportDestroy() ========
 *
 * This function used to destroy SIP service transport.
 *
 * Returns:
 *   SAPP_ERR : Transport failed to initialize.
 *   SAPP_OK : Transport initialized successfully.
 *
 */
vint SAPP_sipServiceTransportDestroy(
    SAPP_ServiceObj  *service_ptr)
{
    if (OSAL_SUCCESS == OSAL_netIsSocketIdValid(&service_ptr->sipInfcFd)) {
        IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcFd);
    }

    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(&service_ptr->sipInfcTcpServerFd)) {
        IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcTcpServerFd);
    }

    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(&service_ptr->sipInfcTcpClientFd)) {
        IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcTcpClientFd);
    }

    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(&service_ptr->sipInfcProtectedServerFd)) {
        IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcProtectedServerFd);
    }

    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(&service_ptr->sipInfcProtectedClientFd)) {
        IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcProtectedClientFd);
    }

    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(
            &service_ptr->sipInfcProtectedTcpServerFd)) {
        IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcProtectedTcpServerFd);
    }

    if (OSAL_SUCCESS ==
            OSAL_netIsSocketIdValid(
            &service_ptr->sipInfcProtectedTcpClientFd)) {
        IMS_NET_CLOSE_SOCKET(&service_ptr->sipInfcProtectedTcpClientFd);
    }

    /* Set flag as OSAL_FALSE */
    service_ptr->isTransportReady = OSAL_FALSE;
    return (SAPP_OK);
}

/*
 * ======== SAPP_sipServiceNicErr() ========
 *
 * This function used to process NIC error event from sip stack.
 * NIC error usually causes by tcp disconnected.
 *
 * Returns:
 *   SAPP_ERR : Transport failed to initialize.
 *   SAPP_OK : Transport initialized successfully.
 *
 */
vint SAPP_sipServiceNicErr(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr)
{
    vint         originalPort;

    if (eTransportUdp == service_ptr->sipTransportType) {
        /* Original transport is UDP, just re-init the transport. */
        return (SAPP_sipServiceTransportInit(service_ptr, sip_ptr));
    }
    
    /*
     * Then it's TCP or TLS. 
     * De-register registration of original port.
     */
    originalPort =  service_ptr->sipConfig.localConn.addr.port;
    /* Clean original transport and re-init transport. */
    if (SAPP_OK != SAPP_sipServiceTransportInit(service_ptr, sip_ptr)) {
        return (SAPP_ERR);
    }
    /* Set original port to the connection. */
    service_ptr->registration.lclConn.addr.port = originalPort;

    /* Run state machine to de-register. */
    SAPP_regRestart(&service_ptr->registration, service_ptr, &sip_ptr->event,
            sip_ptr);

    return (SAPP_OK);
}

/*
 * ======== SAPP_sipServiceTransportSwitch() ========
 *
 * This function disable/enable a file descriptor used for reading
 * data from the network interface thread and for sending SIP
 * message to the network interface.
 *
 * Returns:
 *   None.
 */
void SAPP_sipServiceTransportSwitch(
    SAPP_ServiceObj  *service_ptr,
    vint            onOff)
{
    if (OSAL_SUCCESS == OSAL_netIsSocketIdValid(&service_ptr->sipInfcFd)) {
        SIP_SwitchNetworkInterface(service_ptr->sipInfcFd, onOff);
    }
}

/*
 * ======== SAPP_sipServiceDestroyAll() ========
 *
 * This function will search for all valid service and destory them all.
 * This function will be used when shutdown sapp.
 *
 * Returns:
 *   None.
 */
void SAPP_sipServiceDestroyAll(
    SAPP_SipObj *sip_ptr)
{
    vint x;

    for (x = 0 ; x < SAPP_SIP_MAX_UA ; x++) {
        if (0 !=sip_ptr->service[x].isiServiceId) {
            _SAPP_sipServiceDestroy(&sip_ptr->service[x]);
        }
    }
    return;
}

/*
 * ======== SAPP_sipEvent() ========
 *
 * This function is the entry point for events that come from the SIP Stack.
 *
 * This function is the first place SIP events, destined for the SIP
 * application, are handled.
 *
 * sip_ptr : A pointer to the SIP object associated with this event.
 *
 * evt_ptr : A pointer to a tUaAppEvent object that is populated with all the
 *       details of the event.
 *
 * Returns:
 *   SIP_OK : Always.
 */
vint SAPP_sipEvent(
    SAPP_SipObj   *sip_ptr,
    tUaAppEvent   *evt_ptr)
{
    SAPP_ServiceObj *service_ptr;
    ISIP_Message    *isi_ptr;
    vint             isNewRegistration;
 
    if (NULL == (service_ptr = _SAPP_findServiceViaUaId(sip_ptr,
            evt_ptr->header.hUa))) {
        /* No service, ignore this event */
        return (SIP_FAILED);
    }

    /* Setup the isi_ptr to a block or memory used for
     * constructing ISI events
     */
    isi_ptr = &sip_ptr->event.isiMsg;

    switch (evt_ptr->header.type) {
    case eUA_REGISTRATION_COMPLETED:
        if (SAPP_OK == SAPP_regEvent(&service_ptr->registration, service_ptr,
                evt_ptr, &sip_ptr->event, &isNewRegistration, sip_ptr)) {
            /* Then the SIP event was handled by the SAPP_reg package */

 #ifdef INCLUDE_SIMPLE
            if (SAPP_OK == isNewRegistration) {
                /*
                 * If this is a new registration, then let's init the
                 * SIMPLE protocol stuff to.
                 */
                if (OSAL_TRUE == service_ptr->simple.useStorageServer) {
                    SIMPL_start(sip_ptr, &service_ptr->simple, &sip_ptr->xcap, service_ptr,
                            &sip_ptr->xcap.lists,
                            &sip_ptr->xcap.contacts);
                }
            }
#endif
        }
        break;
    case eUA_REGISTRATION_FAILED:
        SAPP_regEvent(&service_ptr->registration, service_ptr,
            evt_ptr, &sip_ptr->event, &isNewRegistration, sip_ptr);
        break;
    case eUA_TEXT_MESSAGE:
    case eUA_TEXT_MESSAGE_COMPLETED:
    case eUA_TEXT_MESSAGE_FAILED:

#ifdef SIP_CUSTOM_CAPABILITY_EXCHANGE
        /**
         * Note sip2sip.info does not support using SIP OPTIONs for capability exchanges.
         * vPort can be built so when running on sip2sip.info network the
         * capability exchanges occur over SIP MESSAGE requests/responses
         * rather than using SIP OPTION request/responses.  The routine
         * below will check to see if an incoming text message request/response
         * is for capability exchanges.
         */
        if (SAPP_OK == _SAPP_capabilitiesMessageEvent(service_ptr, evt_ptr, 
                &sip_ptr->event)) {
            break;
        }
#endif
        /* Pass it through the Message Waiting Indication machinery */
        if (SAPP_OK == SAPP_mwiEvent(service_ptr, evt_ptr, &sip_ptr->event,
                sip_ptr)) {
            /* Then it was processed.  Nothing else to do */
            break;
        }        
        /* Check if it's a response to call settings */
        if (SAPP_OK ==  SAPP_callSettingsEvent(
                service_ptr, evt_ptr, &sip_ptr->event)) {
            break;
        }
        /* Pass it through the Page mode sub-module */
        if (SAPP_OK == SAPP_imEvent(service_ptr, evt_ptr, &sip_ptr->event,
                sip_ptr)) {
            /* Then it was processed Nothing more to do. */
            break;
        }
        break;
    case eUA_RESPONSE:
    case eUA_ERROR:
    case eUA_RINGING:
    case eUA_CALL_IS_BEING_FORW:
    case eUA_CALL_ATTEMPT:
    case eUA_ANSWERED:
    case eUA_CALL_DROP:
    case eUA_MEDIA_INFO:
    case eUA_TRANSFER_ATTEMPT:
    case eUA_TRANSFER_RINGING:
    case eUA_TRANSFER_COMPLETED:
    case eUA_TRANSFER_FAILED:
    case eUA_UPDATE:
    case eUA_UPDATE_FAILED:
    case eUA_UPDATE_COMPLETED:
    case eUA_SESSION:
    case eUA_PRACK:
    case eUA_PRACK_FAILED:
    case eUA_PRACK_COMPLETED:
    case eUA_ACK:
    case eUA_CANCELED:
#ifdef INCLUDE_SIMPLE
        /* See if the event belongs to the SIMPLE IM Session module */
        if (SAPP_OK == SIMPL_fileTransferEvent(sip_ptr, service_ptr,
                evt_ptr, &sip_ptr->event)) {
            /*
             * Then this event was handled by the SIMPLE FT session support.
             * No further processing of the events should happen.
             */
            break;
        }

        /* See if the event belongs to the SIMPLE IM Session module */
        if (SAPP_OK == SIMPL_imSessionEvent(sip_ptr, service_ptr,
                evt_ptr, &sip_ptr->event)) {
            /* 
             * Then this event was handled by the SIMPLE IM session support.
             * No further processing of the events should happen.
             */
            break;
        }
#endif
        if (SAPP_OK == SAPP_sipUssdEvent(sip_ptr, service_ptr, evt_ptr->header.hUa,
                evt_ptr->header.hOwner, evt_ptr->header.type,
                evt_ptr, &sip_ptr->event)) {
            break;
        }
        /*
         * If it's an event related to "dialog" control (a.k.a.'a phone call')
         * then call the interface in the dialog" file.
         */

        SAPP_sipCallEvent(sip_ptr, service_ptr, evt_ptr->header.hUa,
                evt_ptr->header.hOwner, evt_ptr->header.type,
                evt_ptr, &sip_ptr->event);


        break;

    case eUA_INFO_FAILED:

    case eUA_INFO_COMPLETED:
    case eUA_INFO:
        if (SAPP_OK == SAPP_sipUssdEvent(sip_ptr, service_ptr, evt_ptr->header.hUa,
                evt_ptr->header.hOwner, evt_ptr->header.type,
                evt_ptr, &sip_ptr->event)) {
            break;
        }
        /* Check and see if it's a telephone event */
        if (SAPP_OK == SAPP_teEvent(&service_ptr->telEvt, service_ptr, evt_ptr,
                &sip_ptr->event)) {
            break;
        }
        /* Add more INFO handlers here */
        SAPP_sipCallEvent(sip_ptr, service_ptr, evt_ptr->header.hUa,
                evt_ptr->header.hOwner, evt_ptr->header.type,
                evt_ptr, &sip_ptr->event);
        break;

    case eUA_SUBSCRIBE:
    case eUA_SUBSCRIBE_COMPLETED:
    case eUA_SUBSCRIBE_FAILED:
    case eUA_SUBSCRIBE_FAILED_NO_SUBS:
    case eUA_NOTIFY_EVENT_NO_SUBS:
    case eUA_NOTIFY_EVENT_COMPLETED:
    case eUA_NOTIFY_EVENT_FAILED:
    case eUA_NOTIFY_EVENT:
#ifdef INCLUDE_SIMPLE
        /* Pass it through the SIMPLE sub-module */
        if (SAPP_OK == SIMPL_subEvent(service_ptr, evt_ptr, &sip_ptr->event)) {
            /* Then it was processed Nothing more to do. */
            break;
        }
#endif

        /*
         * See if the event belongs to the a session for managing the
         * conference event package.
         */
        if (SAPP_OK == SAPP_conferenceSubscribeEvent(service_ptr, evt_ptr,
                &sip_ptr->event, sip_ptr)) {
            /* Then it was processed */
            break;
        }

        /* Pass it through the Message Waiting Indication machinery */
        if (SAPP_OK == SAPP_mwiEvent(service_ptr, evt_ptr, &sip_ptr->event, sip_ptr)) {
            /* Then it was processed.  Nothing else to do */
            break;
        }

        if (SAPP_OK == SAPP_regEvent(&service_ptr->registration,
                service_ptr, evt_ptr, &sip_ptr->event, &isNewRegistration, sip_ptr)) {
            /* Then it was processed.  Nothing else to do */
            break;
        }

        /* Add more subscription/notification handlers here */
        break;

    case eUA_PUBLISH:
    case eUA_PUBLISH_COMPLETED:
    case eUA_PUBLISH_FAILED:
#ifdef INCLUDE_SIMPLE
        /* Pass it through the SIMPLE sub-module */
        if (SAPP_OK == SIMPL_pubEvent(service_ptr, evt_ptr, &sip_ptr->event)) {
            /* Then it was processed Nothing more to do. */
            break;
        }
#endif
        /* Add more PUBLISH handlers here */
        break;

    case eUA_OPTIONS:
    case eUA_OPTIONS_COMPLETED:
    case eUA_OPTIONS_FAILED:

        _SAPP_capabilitiesOptionsEvent(service_ptr,
                isi_ptr /* &sip_ptr->event.isiMsg */,
                evt_ptr, &sip_ptr->event, sip_ptr);
        break;
    case eUA_NIC_ERROR:
        /* Process NIC error. */
        if (SAPP_OK != SAPP_sipServiceNicErr(service_ptr, sip_ptr)) {
            /* Failed to process NIC error. Tell ISI */
            OSAL_snprintf(sip_ptr->event.isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ, "REG FAILED: CODE:%d REASON:%s",
                    ISI_IMS_XPORT_INIT_FAILURE, ISI_IMS_XPORT_INIT_FAILURE_STR);
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_NET, ISIP_STATUS_FAILED,
                    &sip_ptr->event.isiMsg);
            OSAL_logMsg("SAPP_sendEvent eUA_NIC_ERROR");
            SAPP_sendEvent(&sip_ptr->event);
            /* Send another event up for deactivate */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &sip_ptr->event.isiMsg);
            /* Run state machine to reset. */
            SAPP_regReset(&service_ptr->registration, service_ptr,
                    &sip_ptr->event, sip_ptr);
        }
        break;
    case eUA_LAST_EVENT:
        default:
            break;
    } /* End of the switch statement */
    return (SIP_OK);
}

