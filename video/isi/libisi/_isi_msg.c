/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30028 $ $Date: 2014-11-21 19:05:32 +0800 (Fri, 21 Nov 2014) $
 *
 */

#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_queue.h"
#include "_isi_service.h"
#include "_isi_call.h"
#include "_isi_msg.h"
#include "_isi_dbg.h"

/*
 * ======== ISIM_alloc() ========
 * A function to specifically alloc a ISIP_message object.
 * Note, that memory is also zero'd here.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a ISIP_Message object
 */
ISIP_Message *ISIM_alloc(void)
{
    ISIP_Message *msg_ptr;
    msg_ptr = ISI_alloc(sizeof(ISIP_Message), ISI_OBJECT_ISIP_MSG);
    if (msg_ptr) {
        OSAL_memSet(msg_ptr, 0, sizeof(ISIP_Message));
    }
    return msg_ptr;
}

/*
 * ======== ISIM_free() ========
 * A function to specifically free a ISIP_message object.
 *
 * Returns:
 *  Nothing
 */
void ISIM_free(ISIP_Message *msg_ptr)
{
    ISI_free(msg_ptr, ISI_OBJECT_ISIP_MSG);
}

/*
 * ======== _ISIM_header() ========
 * A function to that populates a ISIP_Message object header
 *
 * Returns:
 *  Nothing
 */
static void _ISIM_header(
    ISIP_Message      *msg_ptr,
    ISI_Id             id,
    ISIP_Code          code,
    vint               protocol)
{
    msg_ptr->id = id;
    msg_ptr->code = code;
    msg_ptr->protocol = protocol;
}

/*
 * ======== _ISIM_serviceHeader() ========
 * A function to that populates a ISIP_Message object with service
 * header information.
 *
 * Returns:
 *  Nothing
 */
static void _ISIM_serviceHeader(
    ISID_ServiceId    *service_ptr,
    ISIP_Message      *msg_ptr,
    ISIP_ServiceReason reason,
    ISI_ServerType     server)
{
    ISIP_Service *s_ptr;

    /* The id for the underlying protocol to use as the service descriptor */
    _ISIM_header(msg_ptr, service_ptr->e.desc, ISIP_CODE_SERVICE,
            service_ptr->protocol);
    s_ptr = &msg_ptr->msg.service;
    s_ptr->reason = reason;
    s_ptr->status = ISIP_STATUS_TRYING;
    s_ptr->server = server;
    return;
}

/*
 * ======== ISIM_activation() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to indicate to
 * 'activate' de-active' a service.
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * activate : '1' indicates to 'activate'. '0' indicates to 'deactivate'
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_activation(
    ISID_ServiceId *service_ptr,
    vint            activate)
{
    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    if (activate) {
        _ISIM_serviceHeader(service_ptr, msg_ptr,
                 ISIP_SERVICE_REASON_ACTIVATE, ISI_SERVER_TYPE_INVALID);
    }
    else {
        _ISIM_serviceHeader(service_ptr, msg_ptr,
                 ISIP_SERVICE_REASON_DEACTIVATE, ISI_SERVER_TYPE_INVALID);
    }

   return msg_ptr;
}

/*
 * ======== ISIM_block() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to indicate to
 * that the caller would like to 'block' or 'unblock a user from a
 * service.
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * userName_ptr : A NULL terminated string representing the URI (JID)
 *                of the user to block.
 *
 * block : '1' indicates to 'block'. '0' indicates to 'unblock'
 *         the user specified in userName_ptr.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_block(
    ISID_ServiceId *service_ptr,
    char           *userName_ptr,
    vint            block)
{
    ISIP_Message *msg_ptr;
    ISIP_Service *s_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_BLOCKUSER, ISI_SERVER_TYPE_INVALID);

    s_ptr = &msg_ptr->msg.service;

    OSAL_strncpy(s_ptr->settings.uri, userName_ptr, ISI_ADDRESS_STRING_SZ);

    if (block) {
        s_ptr->status = ISIP_STATUS_TRYING;
    }
    else {
        s_ptr->status = ISIP_STATUS_DONE;
    }
    return msg_ptr;
}

/*
 * ======== ISIM_forward() ========
 * A function populates a ISIP_Message object with data necessary to indicate
 * that the service wishes to set conditional call forwarding.
 *
 * evt_ptr : A pointer to a ISID_EvtId object that is used to manage this
 *           command.
 *
 * condition : An enumerated value representing which call forwarding
 *             condition to set.
 *
 * enable : 1 = enable the conditional call forwarding, 0 = disable the
 *          conditional call forwarding.
 *
 * to_ptr : A NULL terminated string representing the forwarding target.
 *
 * timeout : This value is only applicable to the ISI_FORWARD_NO_REPLY
 *           condition.  It represents the timeout is seconds before forwarding
 *           the call.  For all other call forwarding conditions this value
 *           should be ignored.
 *
 * Returns:
 *  Nothing.
 */
void ISIM_forward(
    ISID_EvtId     *evt_ptr,
    ISI_FwdCond     condition,
    int             enable,
    char           *to_ptr,
    int             timeout)
{

    ISIP_Message   *m_ptr;
    ISIP_TelEvent  *e_ptr;

    m_ptr = &evt_ptr->isiMsg;

    /* The id for the underlying protocol to use is the tel event Id */
    _ISIM_header(m_ptr, evt_ptr->e.desc, ISIP_CODE_TEL_EVENT,
            evt_ptr->service_ptr->protocol);

    e_ptr = &m_ptr->msg.event;
    e_ptr->reason = ISIP_TEL_EVENT_REASON_NEW;
    e_ptr->evt = ISI_TEL_EVENT_CALL_FORWARD;
    e_ptr->serviceId = evt_ptr->service_ptr->e.desc;
    e_ptr->callId = 0;
    e_ptr->settings.forward.condition = condition;
    e_ptr->settings.forward.enable = enable;
    e_ptr->settings.forward.timeout = timeout;

    if (0 != enable) {
        OSAL_strncpy(e_ptr->to, to_ptr, ISI_ADDRESS_STRING_SZ);
    }
    else {
        e_ptr->to[0] = 0;
    }
    return;
}

void ISIM_sendUSSD(
    ISID_EvtId     *evt_ptr,
    char           *ussd)
{

    ISIP_Message   *m_ptr;
    ISIP_TelEvent  *e_ptr;

    m_ptr = &evt_ptr->isiMsg;

    /* The id for the underlying protocol to use is the tel event Id */
    _ISIM_header(m_ptr, evt_ptr->e.desc, ISIP_CODE_TEL_EVENT,
            evt_ptr->service_ptr->protocol);

    e_ptr = &m_ptr->msg.event;
    e_ptr->reason = ISIP_TEL_EVENT_REASON_NEW;
    e_ptr->evt = ISI_TEL_EVENT_SEND_USSD;
    e_ptr->serviceId = evt_ptr->service_ptr->e.desc;
    e_ptr->callId = 0;
    OSAL_strncpy(e_ptr->to, ussd, ISI_ADDRESS_STRING_SZ);
    return;
}

void ISIM_getServiceAttribute(
    ISID_EvtId          *evt_ptr,
    ISI_SeviceAttribute  cmd,
    char                *arg1,
    char                *arg2)
{

    ISIP_Message   *m_ptr;
    ISIP_TelEvent  *e_ptr;

    m_ptr = &evt_ptr->isiMsg;

    /* The id for the underlying protocol to use is the tel event Id */
    _ISIM_header(m_ptr, evt_ptr->e.desc, ISIP_CODE_TEL_EVENT,
            evt_ptr->service_ptr->protocol);

    e_ptr = &m_ptr->msg.event;
    e_ptr->reason = ISIP_TEL_EVENT_REASON_NEW;
    e_ptr->serviceId = evt_ptr->service_ptr->e.desc;
    e_ptr->callId = 0; /* Don't need */
    e_ptr->evt = ISI_TEL_EVENT_GET_SERVICE_ATTIBUTE;

    e_ptr->settings.service.cmd = cmd;

    OSAL_strncpy(e_ptr->settings.service.arg1, arg1, 
            ISI_SECURITY_KEY_STRING_SZ);
    OSAL_strncpy(e_ptr->settings.service.arg2, arg2, 
            ISI_SECURITY_KEY_STRING_SZ);

    return;
}


/*
 * ======== ISIM_service() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to indicate to
 * that the caller would like to create/destroy a service.
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * create : '1' indicates to 'create'. '0' indicates to 'destroy'
 *          the service specified in service_ptr.
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_service(
    ISID_ServiceId *service_ptr,
    vint            create)
{
    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    if (create) {
        _ISIM_serviceHeader(service_ptr, msg_ptr,
                 ISIP_SERVICE_REASON_CREATE, ISI_SERVER_TYPE_INVALID);
        /* Add the country */
        OSAL_strncpy(msg_ptr->msg.service.settings.country,
                service_ptr->szCountry, ISI_COUNTRY_STRING_SZ);
    }
    else {
        _ISIM_serviceHeader(service_ptr, msg_ptr,
                 ISIP_SERVICE_REASON_DESTROY, ISI_SERVER_TYPE_INVALID);
    }
    return msg_ptr;
}

/*
 * ======== ISIM_server() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to specify 'servers'
 * for a particular service
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * server : An enumerated value representing the server to set.
 *          see the ISI_ServerType definition for all possible values
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_server(
    ISID_ServiceId *service_ptr,
    ISI_ServerType  server)
{
    char         *from_ptr;
    ISIP_Service *s_ptr;

    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_SERVER, server);

    s_ptr = &msg_ptr->msg.service;

    switch (server) {
    case ISI_SERVER_TYPE_STUN:
        from_ptr = service_ptr->szStunServer;
        break;
    case ISI_SERVER_TYPE_PROXY:
        from_ptr = service_ptr->szProxy;
        break;
    case ISI_SERVER_TYPE_REGISTRAR:
        from_ptr = service_ptr->szRegistrar;
        break;
    case ISI_SERVER_TYPE_OUTBOUND_PROXY:
        from_ptr = service_ptr->szOutboundProxy;
        break;
    case ISI_SERVER_TYPE_RELAY:
        from_ptr = service_ptr->szRelayServer;
        break;
    case ISI_SERVER_TYPE_STORAGE:
        from_ptr = service_ptr->szStorageServer;
        break;
    case ISI_SERVER_TYPE_CHAT:
        from_ptr = service_ptr->szChatServer;
        break;
    default:
        ISIM_free(msg_ptr);
        return (NULL);
    }
    OSAL_strncpy(s_ptr->settings.server, from_ptr, ISI_LONG_ADDRESS_STRING_SZ);
    return msg_ptr;
}

/*
 * ======== ISIM_port() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to specify port and pool size
 * for a particular service
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * server : An enumerated value representing the port to set.
 *          see the ISI_PortType definition for all possible values
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_port(
    ISID_ServiceId *service_ptr,
    ISI_PortType    portType)
{
    ISIP_Service *s_ptr;

    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_PORT, ISI_SERVER_TYPE_INVALID);

    s_ptr = &msg_ptr->msg.service;

    s_ptr->settings.port.portType = portType;
    switch (portType) {
    case ISI_PORT_TYPE_SIP:
        s_ptr->settings.port.portNum = service_ptr->sipPort;
        break;
    case ISI_PORT_TYPE_AUDIO:
        s_ptr->settings.port.portNum = service_ptr->audioRtpPort;
        s_ptr->settings.port.poolSize = service_ptr->audioPoolSize;
        break;
    case ISI_PORT_TYPE_VIDEO:
        s_ptr->settings.port.portNum = service_ptr->videoRtpPort;
        s_ptr->settings.port.poolSize = service_ptr->videoPoolSize;
        break;
    default:
        ISIM_free(msg_ptr);
        return (NULL);
    }

    return msg_ptr;
}

ISIP_Message* ISIM_filePath(
    ISID_ServiceId *service_ptr,
    char           *filePath_ptr,
    char           *filePrepend_ptr)
{
    ISIP_Message *msg_ptr;
    ISIP_Service *s_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    s_ptr = &msg_ptr->msg.service;

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_FILE, ISI_SERVER_TYPE_INVALID);

    if (NULL != filePath_ptr) {
        OSAL_strncpy(s_ptr->settings.file.filePath,
                filePath_ptr, ISI_FILE_PATH_STRING_SZ);
    }
    if (NULL != filePrepend_ptr) {
        OSAL_strncpy(s_ptr->settings.file.filePrepend,
                filePrepend_ptr, ISI_ADDRESS_STRING_SZ);
    }
    return msg_ptr;
}

/*
 * ======== ISIM_bsid() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to specify a BSID
 * for a particular service
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_bsid(
    ISID_ServiceId *service_ptr)
{
    ISIP_Service *s_ptr;
    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_BSID, ISI_SERVER_TYPE_INVALID);

    s_ptr = &msg_ptr->msg.service;
    /* populate the BSID */
    OSAL_strncpy(s_ptr->settings.bsId.szBsId,
            service_ptr->bsId.szBsId,
            ISI_BSID_STRING_SZ);
    s_ptr->settings.bsId.type = service_ptr->bsId.type;
    return msg_ptr;

}

/*
 * ======== ISIM_imei() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to specify a IMEI
 * for a particular service
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_imei(
    ISID_ServiceId *service_ptr)
{
    ISIP_Service *s_ptr;
    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_IMEI_URI, ISI_SERVER_TYPE_INVALID);

    s_ptr = &msg_ptr->msg.service;
    /* populate the IMEI */
    OSAL_strncpy(s_ptr->settings.imeiUri, service_ptr->imeiUri, ISI_ADDRESS_STRING_SZ);
    return msg_ptr;
}

/*
 * ======== ISIM_ipsec() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to specify IPSec port and SPI
 * infomation for a particular service
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_ipsec(
    ISID_ServiceId *service_ptr)
{
    ISIP_Service *s_ptr;
    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_IPSEC, ISI_SERVER_TYPE_INVALID);

    s_ptr = &msg_ptr->msg.service;
    /* populate IPSec info */
    s_ptr->settings.ipsec.cfg.protectedPort       = service_ptr->protectedPort;
    s_ptr->settings.ipsec.cfg.protectedPortPoolSz =
            service_ptr->protectedPortPoolSz;
    s_ptr->settings.ipsec.cfg.spi                 = service_ptr->spi;
    s_ptr->settings.ipsec.cfg.spiPoolSz           = service_ptr->spiPoolSz;

    return msg_ptr;
}

/*
 * ======== ISIM_uri() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to specify a URI
 * for a particular service
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_uri(
    ISID_ServiceId *service_ptr)
{
    ISIP_Service *s_ptr;
    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_URI, ISI_SERVER_TYPE_INVALID);

    s_ptr = &msg_ptr->msg.service;
    /* populate the uri */
    OSAL_strncpy(s_ptr->settings.uri, service_ptr->szUri, ISI_ADDRESS_STRING_SZ);
    return msg_ptr;

}

/*
 * ======== ISIM_instanceId() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to specify the device Id
 * for a particular service
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_instanceId(
    ISID_ServiceId *service_ptr)
{
    ISIP_Service *s_ptr;
    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_INSTANCEID, ISI_SERVER_TYPE_INVALID);

    s_ptr = &msg_ptr->msg.service;
    /* populate the uri */
    OSAL_strncpy(s_ptr->settings.instanceId, service_ptr->szInstanceId,
            (ISI_INSTANCE_STRING_SZ + 1));
    return msg_ptr;

}

/*
 * ======== ISIM_emergency() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to specify a isEmergency
 * for a particular service
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_emergency(
    ISID_ServiceId *service_ptr)
{
    ISIP_Service *s_ptr;
    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_EMERGENCY, ISI_SERVER_TYPE_INVALID);

    s_ptr = &msg_ptr->msg.service;
    /* populate the isEmergency */
    s_ptr->settings.isEmergency = service_ptr->isEmergency;
    return msg_ptr;

}

/*
 * ======== ISIM_credentials() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to specify authentication
 * credentials for a particular service
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * acct_ptr :  A pointer to a ISID_Account object that contains the details
 *             of the authentication credentials (i.e.username, password, etc)
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_credentials(
    ISID_ServiceId *service_ptr,
    ISID_Account   *acct_ptr)
{
    ISIP_Service *s_ptr;
    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    if (acct_ptr == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_AUTH, ISI_SERVER_TYPE_INVALID);

    s_ptr = &msg_ptr->msg.service;

    /* Then populate the credentials using the top credential */
    OSAL_strncpy(s_ptr->settings.credentials.username,
                acct_ptr->szUsername, ISI_ADDRESS_STRING_SZ);

    OSAL_strncpy(s_ptr->settings.credentials.password,
                acct_ptr->szPassword, ISI_ADDRESS_STRING_SZ);

    OSAL_strncpy(s_ptr->settings.credentials.realm,
                acct_ptr->szRealm, ISI_ADDRESS_STRING_SZ);

    return msg_ptr;
}

/*
 * ======== ISIM_coders() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to specify the coders
 * supported for a particular service.
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_coders(
    ISID_ServiceId *service_ptr)
{
    ISIP_Service *s_ptr;
    vint          x, y;
    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
         ISIP_SERVICE_REASON_CODERS, ISI_SERVER_TYPE_INVALID);

    s_ptr = &msg_ptr->msg.service;

    for (x = 0, y = 0; x < ISI_CODER_NUM ; x++) {
        if (service_ptr->coders[x].szCoderName[0] != 0) {
            s_ptr->settings.coders[y] = service_ptr->coders[x];
            y++;
        }
    }
    return msg_ptr;
}

/*
 * ======== _ISIM_constructFromField() ========
 * This is a function used to construct an AOR (Address of Record) or JID
 * for a particular service based on the authentication credentials provided
 * for the service.
 *
 * username_ptr : A pointer to a NULL terminated string representing a service's
 *               username.
 *
 * realm_ptr : A pointer to a NULL terminated string representing the "realm"
 *             (or domain) that the service belongs to.
 *
 * out_ptr : A pointer to a buffer where this function will write the AOR
 *           (or JID) of the result of the userName_ptr and realm_ptr.
 *
 * maxOutLen : The max length of the buffer specified in out_ptr.
 *
 * Returns:
 *  ISI_RETURN_OK       : The AOR (JID) was correctly constructed.
 *  ISI_RETURN_FAILED   : The AOR (JID) could not be properly constructed.
 */
static ISI_Return _ISIM_constructFromField(
    char           *username_ptr,
    char           *realm_ptr,
    char           *out_ptr,
    vint            maxOutLen)
{
    vint len;
    if (username_ptr == 0 || realm_ptr == 0 || maxOutLen == 0) {
        return (ISI_RETURN_FAILED);
    }
    len = OSAL_strlen(username_ptr);
    /* Copy the username so you have 'sip:myaccount' */
    OSAL_strncpy(out_ptr, username_ptr, maxOutLen);
    out_ptr += len;
    maxOutLen -= len;
    if (maxOutLen <= 1 ) {
        /* Then we exceeded the total length */
        return (ISI_RETURN_FAILED);
    }
    *out_ptr = '@';
     out_ptr++;
     maxOutLen -= 1;
     /* Now you have 'sip:myaccount@' so copy the realm */
     len = OSAL_strlen(realm_ptr);
     OSAL_strncpy(out_ptr, realm_ptr, maxOutLen);
     maxOutLen -= len;
     if (maxOutLen < 0 ) {
         /* Then we exceeded the total length */
         return (ISI_RETURN_FAILED);
     }
     return (ISI_RETURN_OK);
}

/*
 * ======== ISIM_privateCid() ========
 * A function that allocates a ISIP_Message object and populates the object
 * with data necessary to indicate to that the service would like to block
 * caller ID data when placing a call.
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * makePrivate : '1' indicates to 'make caller ID private'. '0' indicates to
 *               include caller ID information when placing a call.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_privateCid(
    ISID_ServiceId *service_ptr,
    vint            makePrivate)
{
    ISIP_Service *s_ptr;
    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
         ISIP_SERVICE_REASON_IDENTITY, ISI_SERVER_TYPE_INVALID);

    s_ptr = &msg_ptr->msg.service;
    s_ptr->settings.identityHide = makePrivate;
    return msg_ptr;
}

/*
 * ======== _ISIM_populateCallHeader() ========
 * A function populates ISID_Message objects with a call header.
 * It works as a helper function to other ISIM_xxx calls used for
 * placing and modifying calls.
 *
 * Returns:
 *  Nothing
 */
static void _ISIM_populateCallHeader(
    ISIP_Message    *m_ptr,
    vint             protocol,
    ISI_Id           callId,
    ISI_Id           serviceId,
    ISIP_CallReason  reason,
    ISIP_Status      status)
{
    ISIP_Call    *c_ptr;

    _ISIM_header(m_ptr, callId, ISIP_CODE_CALL, protocol);

    c_ptr = &m_ptr->msg.call;
    c_ptr->serviceId = serviceId;
    c_ptr->reason = reason;
    c_ptr->status = status;
}

/*
 * ======== _ISIM_populateCall() ========
 * A function populates a ISIP_Message object with data necessary for placing
 * and controlling calls.
 *
 * Returns:
 *  Nothing
 */
static void _ISIM_populateCall(
    ISIP_Message    *m_ptr,
    ISID_CallId     *call_ptr,
    ISI_Id           serviceId,
    ISIP_CallReason  reason,
    ISIP_Status      status)
{
    ISIP_Call    *c_ptr;

    _ISIM_populateCallHeader(m_ptr, call_ptr->service_ptr->protocol,
            call_ptr->e.desc, serviceId, reason, status);

    c_ptr = &m_ptr->msg.call;

    /* Add the session info */
    c_ptr->type = call_ptr->type;
    c_ptr->lclVideoAsBwKbps = call_ptr->rtpVideoLcl.videoAsBwKbps;
    c_ptr->audioDirection = call_ptr->audioDir;
    c_ptr->videoDirection = call_ptr->videoDir;
    c_ptr->rsrcStatus = call_ptr->rsrcStatus;
    c_ptr->srvccStatus = call_ptr->srvccStatus;
    /* copy the coder info */
    OSAL_memCpy(c_ptr->coders, call_ptr->coders,
            (sizeof(ISIP_Coder) * ISI_CODER_NUM));
}

/*
 * ======== ISIM_initiateCall() ========
 * A function that allocates a ISIP_Message object and populates the object
 * with data necessary to initiate a call to remote entity.
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * call_ptr : A pointer to a ISID_CallId object that includes all the details
 *            regarding the call to place.
 *
 * msg_ptr : An address to a pointer.  If this function is successful then
 *           this value will be populated a pointer to a ISIP_Message object
 *           that is ready to command the underlying protocol.
 *
 * Returns:
 *  ISI_RETURN_OK : The function was successful and the msg_ptr parameter
 *                  is ready to be passed to the underlying protocol.
 * ISI_RETURN_FAILED : The function could not allocate and/or initialize the
 *                     msg_ptr parameter object.
 */
ISI_Return ISIM_initiateCall(
    ISID_ServiceId   *service_ptr,
    ISID_CallId      *call_ptr,
    ISIP_Message    **msg_ptr)
{
    ISIP_Call    *c_ptr;
    ISIP_Message *m_ptr;
    ISID_Account *acct_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (ISI_RETURN_FAILED);
    }

    _ISIM_populateCall(m_ptr, call_ptr, service_ptr->e.desc,
              ISIP_CALL_REASON_INITIATE, ISIP_STATUS_INVALID);

    c_ptr = &m_ptr->msg.call;

    OSAL_strncpy(c_ptr->to, call_ptr->szRemoteUri, ISI_ADDRESS_STRING_SZ);

    acct_ptr = ISID_accountGet(service_ptr);
    /* Construct the 'from' field */
    if (acct_ptr == NULL) {
        ISIM_free(m_ptr);
        return (ISI_RETURN_INVALID_ADDRESS);
    }

    if (_ISIM_constructFromField(
            acct_ptr->szUsername, acct_ptr->szRealm, c_ptr->from,
            ISI_ADDRESS_STRING_SZ) != ISI_RETURN_OK) {
        ISIM_free(m_ptr);
        return (ISI_RETURN_INVALID_ADDRESS);
    }
    c_ptr->cidType = call_ptr->cidType;

    OSAL_strncpy(c_ptr->subject, call_ptr->szSubject, ISI_SUBJECT_STRING_SZ);
    *msg_ptr = m_ptr;
    return (ISI_RETURN_OK);
}

/*
 * ======== ISIM_call() ========
 * A function that populates a ISID_Message object used to return errors
 * back to the underlying protocol when an error is detected with an incoming
 * call request.
 *
 * Returns:
 *  Nothing
 *
 */
void ISIM_call(
    ISIP_Message   *msg_ptr,
    ISIP_CallReason reason,
    ISIP_Status     status)
{
    _ISIM_populateCallHeader(msg_ptr, msg_ptr->protocol, msg_ptr->id,
            msg_ptr->msg.call.serviceId, reason, status);
    return;
}

/*
 * ======== ISIM_updateCall() ========
 * A function that allocates a ISIP_Message object and populates the object
 * with data necessary to 'update' or modify a call.
 *
 * call_ptr : A pointer to a ISID_CallId object that includes all the details
 *            regarding the call to place.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_updateCall(
    ISID_CallId     *call_ptr,
    ISIP_CallReason  reason,
    ISIP_Status      status,
    char            *to_ptr,
    char            *subject_ptr)
{
    ISIP_Call    *c_ptr;
    ISIP_Message *msg_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_populateCall(msg_ptr, call_ptr, call_ptr->service_ptr->e.desc,
            reason, status);

    c_ptr = &msg_ptr->msg.call;

    if (to_ptr) {
        OSAL_strncpy(c_ptr->to, to_ptr, ISI_ADDRESS_STRING_SZ);
    }
    if (subject_ptr) {
        OSAL_strncpy(c_ptr->subject, subject_ptr, ISI_SUBJECT_STRING_SZ);
    }
    return (msg_ptr);
}

/*
 * ======== ISIM_sendText() ========
 * A function that populates an ISIP_Message object with data necessary to send
 * a text message.
 *
 * text_ptr : A pointer to ISID_TextId object that contains all the details
 *            regarding the text message to sent.

 * serviceId : A unique identifier representing the service that the text
 *             message will be sent within.
 *
 * chatId : A unique identifier representing the chat room that the text
 *          message will be sent within.  If Zero, then the text message is
 *          not associated with any specific grou pchat room.
 *
 * Returns:
 *  Nothing.
 */
void ISIM_sendText(
    ISIP_TextReason reason,
    ISID_TextId    *text_ptr,
    ISI_Id          serviceId,
    ISI_Id          chatId)
{
    ISIP_Text    *t_ptr;
    ISIP_Message *m_ptr;

    m_ptr = &text_ptr->isiMsg;

    /* The id for the underlying protocol to use is the text id */
    _ISIM_header(m_ptr, text_ptr->e.desc, ISIP_CODE_MESSAGE,
            text_ptr->service_ptr->protocol);

    t_ptr = &m_ptr->msg.message;
    t_ptr->reason = reason;
    t_ptr->serviceId = serviceId;
    t_ptr->chatId = chatId;
    /* The actual text message details have already been written */
    return;
}

/*
 * ======== ISIM_sendFile() ========
 * A function that populates an ISIP_File object with data necessary to send
 * a file transfer request.
 *
 * file_ptr : A pointer to ISID_FileId object that contains all the details
 *            regarding the file to sent.

 * serviceId : A unique identifier representing the service that the file
 *             will be sent within.
 *
 * Returns:
 *  Nothing.
 */
void ISIM_sendFile(
    ISID_FileId    *file_ptr,
    ISI_Id          serviceId)
{
    ISIP_File    *f_ptr;
    ISIP_Message *m_ptr;

    m_ptr = &file_ptr->isiMsg;

    /* The id for the underlying protocol to use is the text id */
    _ISIM_header(m_ptr, file_ptr->e.desc, ISIP_CODE_FILE,
            file_ptr->service_ptr->protocol);

    f_ptr = &m_ptr->msg.file;
    f_ptr->reason = ISIP_FILE_REASON_NEW;
    f_ptr->serviceId = serviceId;
    /* The actual file transfer details have already been written */
    return;
}

/*
 * ======== ISIM_acceptFileTransfer() ========
 * A function that populates an ISIP_File object with data necessary to accept
 * a file transfer request.
 *
 * file_ptr : A pointer to ISID_FileId object that contains all the details
 *            regarding the file that we've accepted to receive.
 *
 * Returns:
 *  Nothing.
 */
void ISIM_acceptFileTransfer(
    ISID_FileId    *file_ptr)
{
    ISIP_File    *f_ptr;
    ISIP_Message *m_ptr;

    m_ptr = &file_ptr->isiMsg;

    /* The id for the underlying protocol to use is the text id */
    _ISIM_header(m_ptr, file_ptr->e.desc, ISIP_CODE_FILE,
            file_ptr->service_ptr->protocol);

    f_ptr = &m_ptr->msg.file;
    f_ptr->reason = ISIP_FILE_REASON_ACCEPT;
    //f_ptr->serviceId = serviceId;
    /* The actual file transfer details have already been written */
    return;
}

/*
 * ======== ISIM_beginSendingFileTransfer() ========
 * A function that populates an ISIP_File object with data necessary to begin
 * sending the data for a file transfer.  This should be done only after an
 * MSRP session has been established.
 *
 * file_ptr : A pointer to ISID_FileId object that contains all the details
 *            regarding the file that we've accepted to receive.
 *
 * Returns:
 *  Nothing.
 */
void ISIM_beginSendingFileTransfer(
    ISID_FileId    *file_ptr)
{
    ISIP_File    *f_ptr;
    ISIP_Message *m_ptr;

    m_ptr = &file_ptr->isiMsg;

    /* The id for the underlying protocol to use is the text id */
    _ISIM_header(m_ptr, file_ptr->e.desc, ISIP_CODE_FILE,
            file_ptr->service_ptr->protocol);

    f_ptr = &m_ptr->msg.file;
    f_ptr->reason = ISIP_FILE_REASON_START_SEND;
    //f_ptr->serviceId = serviceId;
    /* The actual file transfer details have already been written */
    return;
}

/*
 * ======== ISIM_acknowledgeFileTransfer() ========
 * A function that populates an ISIP_File object with data necessary to acknowledge
 * a file transfer request.
 *
 * file_ptr : A pointer to ISID_FileId object that contains all the details
 *            regarding the file that we've accepted to receive.
 *
 * Returns:
 *  Nothing.
 */
void ISIM_acknowledgeFileTransfer(
    ISID_FileId    *file_ptr)
{
    ISIP_File    *f_ptr;
    ISIP_Message *m_ptr;

    m_ptr = &file_ptr->isiMsg;

    /* The id for the underlying protocol to use is the text id */
    _ISIM_header(m_ptr, file_ptr->e.desc, ISIP_CODE_FILE,
            file_ptr->service_ptr->protocol);

    f_ptr = &m_ptr->msg.file;
    f_ptr->reason = ISIP_FILE_REASON_ACKNOWLEDGE;
    //f_ptr->serviceId = serviceId;
    /* The actual file transfer details have already been written */
    return;
}

/*
 * ======== ISIM_rejectFileTransfer() ========
 * A function that populates an ISIP_File object with data necessary to reject
 * a file transfer request.
 *
 * file_ptr : A pointer to ISID_FileId object that contains all the details
 *            regarding the file that we've rejected to receive.
 *
 *
 * Returns:
 *  Nothing.
 */
void ISIM_rejectFileTransfer(
    ISID_FileId    *file_ptr,
    const char     *rejectReason_ptr)
{
    ISIP_File    *f_ptr;
    ISIP_Message *m_ptr;

    m_ptr = &file_ptr->isiMsg;

    /* The id for the underlying protocol to use is the file id */
    _ISIM_header(m_ptr, file_ptr->e.desc, ISIP_CODE_FILE,
            file_ptr->service_ptr->protocol);

    f_ptr = &m_ptr->msg.file;
    f_ptr->reason = ISIP_FILE_REASON_REJECT;
    OSAL_strncpy(f_ptr->reasonDesc, rejectReason_ptr, sizeof(f_ptr->reasonDesc));
    return;
}


/*
 * ======== ISIM_cancelFileTransfer() ========
 * A function that populates an ISIP_File object with data necessary to cancel
 * a file transfer request.
 *
 * file_ptr : A pointer to ISID_FileId object that contains all the details
 *            regarding the file.
 *
 *
 * Returns:
 *  Nothing.
 */
void ISIM_cancelFileTransfer(
    ISID_FileId    *file_ptr,
    const char     *cancelReason_ptr)
{
    ISIP_File    *f_ptr;
    ISIP_Message *m_ptr;

    m_ptr = &file_ptr->isiMsg;

    _ISIM_header(m_ptr, file_ptr->e.desc, ISIP_CODE_FILE,
            file_ptr->service_ptr->protocol);

    f_ptr = &m_ptr->msg.file;
    f_ptr->reason = ISIP_FILE_REASON_CANCEL;
    OSAL_strncpy(f_ptr->reasonDesc, cancelReason_ptr, sizeof(f_ptr->reasonDesc));
    return;
}



/*
 * ======== ISIM_shutdownFileTransfer() ========
 * A function that populates an ISIP_File object with data necessary to end
 * a file transfer.  The sending/initiating party of the file transfer
 * may send this to SIMPL to end the dialog after ISIP_FILE_REASON_COMPLETE
 * has been detected.
 *
 * file_ptr : A pointer to ISID_FileId object that contains all the details
 *            regarding the file that we've sent.
 *
 * Returns:
 *  Nothing.
 */
void ISIM_shutdownFileTransfer(
    ISID_FileId    *file_ptr)
{
    ISIP_File    *f_ptr;
    ISIP_Message *m_ptr;

    m_ptr = &file_ptr->isiMsg;

    /* The id for the underlying protocol to use is the text id */
    _ISIM_header(m_ptr, file_ptr->e.desc, ISIP_CODE_FILE,
            file_ptr->service_ptr->protocol);

    f_ptr = &m_ptr->msg.file;
    f_ptr->reason = ISIP_FILE_REASON_SHUTDOWN;
    return;
}


/*
 * ======== ISIM_telEvt() ========
 * A function that populates an ISIP_Message object with data necessary to
 * command the underlying protocol to perform a telephone event.
 *
 * evt_ptr : A pointer to ISID_EvtId object used to manage this telephone event
 *           command.  Note, Currently the only telephone events supported are
 *           DTMF digits.
 *
 * Returns:
 *  Nothing.
 */
void ISIM_telEvt(
    ISID_EvtId  *evt_ptr,
    ISI_TelEvent telEvent,
    const char  *to_ptr,
    int          arg0,
    int          arg1)
{
    ISIP_TelEvent  *e_ptr;
    ISIP_Message   *m_ptr;

    m_ptr = &evt_ptr->isiMsg;

    /* The id for the underlying protocol to use is the tel event Id */
    _ISIM_header(m_ptr, evt_ptr->e.desc, ISIP_CODE_TEL_EVENT,
            evt_ptr->service_ptr->protocol);

    e_ptr = &m_ptr->msg.event;
    e_ptr->reason = ISIP_TEL_EVENT_REASON_NEW;
    e_ptr->evt = telEvent;
    e_ptr->serviceId = evt_ptr->service_ptr->e.desc;
    e_ptr->callId = evt_ptr->call_ptr->e.desc;
    e_ptr->settings.args.arg0 = arg0;
    e_ptr->settings.args.arg1 = arg1;

    if (NULL != to_ptr && 0 != to_ptr[0]) {
        OSAL_strncpy(e_ptr->to, to_ptr, ISI_ADDRESS_STRING_SZ);
    }
    else {
        e_ptr->to[0] = 0;
    }

    return;
}

/*
 * ======== ISIM_telEvtString() ========
 * A function that populates an ISIP_Message object with data necessary to
 * command the underlying protocol to perform a telephone event.
 *
 * evt_ptr : A pointer to ISID_EvtId object used to manage this telephone event
 *           command.  Note, Currently the only telephone events supported are
 *           DTMF digits.
 *
 * Returns:
 *  Nothing.
 */
void ISIM_telEvtString(
    ISID_EvtId  *evt_ptr,
    ISI_TelEvent telEvent,
    const char  *to_ptr,
    const char  *string_ptr,
    int          durationMs)
{
    ISIP_TelEvent  *e_ptr;
    ISIP_Message   *m_ptr;

    m_ptr = &evt_ptr->isiMsg;

    /* The id for the underlying protocol to use is the tel event Id */
    _ISIM_header(m_ptr, evt_ptr->e.desc, ISIP_CODE_TEL_EVENT,
            evt_ptr->service_ptr->protocol);

    e_ptr                     = &m_ptr->msg.event;
    e_ptr->reason             = ISIP_TEL_EVENT_REASON_NEW;
    e_ptr->evt                = telEvent;
    e_ptr->serviceId          = evt_ptr->service_ptr->e.desc;
    e_ptr->callId             = evt_ptr->call_ptr->e.desc;
    e_ptr->settings.args.arg0 = 0; /* Not used */
    e_ptr->settings.args.arg1 = durationMs;

    if (NULL != to_ptr && 0 != to_ptr[0]) {
        OSAL_strncpy(e_ptr->to, to_ptr, ISI_ADDRESS_STRING_SZ);
    }
    else {
        e_ptr->to[0] = 0;
    }

    if (NULL != string_ptr && 0 != string_ptr[0]) {
        OSAL_strncpy(e_ptr->settings.args.string, string_ptr, 
                ISI_ADDRESS_STRING_SZ);
    }
    else {
        e_ptr->settings.args.string[0] = 0;
    }
    return;
}

/*
 * ======== ISIM_pres() ========
 * A function populates a ISIP_Message object with data necessary to send
 * transactions related to presence signalling.
 *
 * pres_ptr : A pointer to ISID_PresId object that contains all the details
 *            regarding the presence event to send.
 *
 * Returns:
 *  Nothing.
 */
void ISIM_pres(
    ISID_PresId     *pres_ptr,
    ISIP_PresReason  reason,
    char            *to_ptr)
{
    ISIP_Presence  *p_ptr;
    ISIP_Message   *m_ptr;

    m_ptr = &pres_ptr->isiMsg;

    /* The id for the underlying protocol to use is the presId */
    _ISIM_header(m_ptr, pres_ptr->e.desc, ISIP_CODE_PRESENCE,
            pres_ptr->service_ptr->protocol);

    p_ptr = &m_ptr->msg.presence;
    p_ptr->reason = reason;
    p_ptr->serviceId = pres_ptr->service_ptr->e.desc;
    p_ptr->chatId = pres_ptr->chatId;
    if (to_ptr) {
        OSAL_strncpy(p_ptr->to, to_ptr, ISI_ADDRESS_STRING_SZ);
    }
    return;
}

/*
 * ======== ISIM_conf() ========
 * A function that allocates a ISIP_Message object and populates the object
 * with data necessary to create a conferenced call.
 *
 * conf_ptr : A pointer to ISID_ConfId object that contains all the details
 *            regarding the calls to conference together.
 *
 * reason : A enumerated value representing "conference command".
 *          possible values are ISIP_MEDIA_REASON_CONFSTART,
 *                              ISIP_MEDIA_REASON_CONFSTOP.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message *ISIM_conf(
    ISID_ConfId     *conf_ptr,
    ISIP_MediaReason reason)
{
    ISID_CallId    *call_ptr;
    uint32          confMask;
    ISIP_Message   *m_ptr;
    ISIP_Conf      *c_ptr;
    vint            x, y;
    vint            protocol;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    m_ptr->msg.media.reason = reason;

    c_ptr = &m_ptr->msg.media.media.conf;

    /* Set the protocol to a default. */
    protocol = ISI_PROTOCOL_VTSP;

    if (reason == ISIP_MEDIA_REASON_CONFSTART) {
        /* Populate with only active calls, or ones with a valid confMask */
        for (x = 0, y = 0 ; x < ISI_CONF_USERS_NUM ; x++) {
            call_ptr = conf_ptr->aCall[x];
            confMask = conf_ptr->aConfMask[x];
            if (call_ptr != NULL && confMask != 0) {
                c_ptr->aCall[y] = call_ptr->e.desc;
                c_ptr->aStreamId[y] = call_ptr->streamId;
                c_ptr->aConfMask[y] = confMask;
                protocol = call_ptr->service_ptr->protocol;
                y++;
            }
        }
    }
    else {
        /* We are removing an element from the conf */
        for (x = 0, y = 0 ; x < ISI_CONF_USERS_NUM ; x++) {
            call_ptr = conf_ptr->aCall[x];
            confMask = conf_ptr->aConfMask[x];
            if (call_ptr != NULL && confMask == 0) {
                c_ptr->aCall[y] = call_ptr->e.desc;
                c_ptr->aStreamId[y] = call_ptr->streamId;
                c_ptr->aConfMask[y] = 0;
                protocol = call_ptr->service_ptr->protocol;
                y++;
            }
        }
    }
    _ISIM_header(m_ptr, conf_ptr->e.desc, ISIP_CODE_MEDIA, protocol);
    return (m_ptr);
}

/*
 * ======== ISIM_stream() ========
 * A function that allocates a ISIP_Message object and populates the object
 * with data necessary to start/modify/stop a media stream.  This function is
 * used to command the VTSP module to start/modify/stop an RTP stream.
 *
 * call_ptr : A pointer to ISID_CallId object that contains all the details
 *            regarding the attributes of the stream, i.e. coders, ip
 *            address and port pairs, security keys, etc.
 *
 * reason : A enumerated value representing "audio command".
 *          possible values are ISIP_MEDIA_REASON_STREAMSTART,
 *                              ISIP_MEDIA_REASON_STREAMSTOP,
 *                              ISIP_MEDIA_REASON_STREAMMODIFY.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message *ISIM_stream(
    ISID_CallId      *call_ptr,
    ISIP_MediaReason  reason)
{
    ISIP_Media    *a_ptr;
    ISIP_Stream   *s_ptr;
    ISIP_Message  *m_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_header(m_ptr, call_ptr->e.desc, ISIP_CODE_MEDIA,
            call_ptr->service_ptr->protocol);

    a_ptr = &m_ptr->msg.media;
    a_ptr->reason = reason;
    a_ptr->serviceId = call_ptr->service_ptr->e.desc;

    s_ptr = &a_ptr->media.stream;
    s_ptr->id = call_ptr->streamId;
    s_ptr->audioDirection = call_ptr->audioDir;
    s_ptr->videoDirection = call_ptr->videoDir;
    s_ptr->confMask = call_ptr->confMask;
    s_ptr->type = call_ptr->type;

    /* Use vtspXXX values for ip addr and port */
    OSAL_netAddrPortCpy(&s_ptr->audio.lclAddr, &call_ptr->rtpAudioLcl.addr);
    s_ptr->audio.lclCntlPort = call_ptr->rtpAudioLcl.cntlPort;

    /* Now the remote values */
    OSAL_netAddrPortCpy(&s_ptr->audio.rmtAddr, &call_ptr->rtpAudioRmt.addr);
    s_ptr->audio.rmtCntlPort = call_ptr->rtpAudioRmt.cntlPort;
    /* Load the username if any */
    OSAL_strncpy(s_ptr->audio.userName, call_ptr->rtpAudioRmt.szUsername,
            ISI_ADDRESS_STRING_SZ);

    /* Load a cname from the this entity's URI */
    OSAL_strncpy(s_ptr->audio.rtcpCname, call_ptr->service_ptr->szUri,
            ISI_ADDRESS_STRING_SZ);
    
    /* Copy any security keys */
    s_ptr->audio.securityKeys = call_ptr->audioKeys;
    
    /* Use vtspXXX values for ip addr and port */
    OSAL_netAddrPortCpy(&s_ptr->video.lclAddr, &call_ptr->rtpVideoLcl.addr);
    s_ptr->video.lclCntlPort = call_ptr->rtpVideoLcl.cntlPort;

    /* Now the remote values */
    OSAL_netAddrPortCpy(&s_ptr->video.rmtAddr, &call_ptr->rtpVideoRmt.addr);
    s_ptr->video.rmtCntlPort = call_ptr->rtpVideoRmt.cntlPort;
    /* Load the username if any */
    OSAL_strncpy(s_ptr->video.userName, call_ptr->rtpVideoRmt.szUsername,
            ISI_ADDRESS_STRING_SZ);

    /* Load a cname from the this entity's URI */
    OSAL_strncpy(s_ptr->video.rtcpCname, call_ptr->service_ptr->szUri,
            ISI_ADDRESS_STRING_SZ);
    
    /* Copy any security keys */
    s_ptr->video.securityKeys = call_ptr->videoKeys;
    
    /* Copy rtcp feedback masks. */
    s_ptr->video.rtcpFbMask = call_ptr->videoRtcpFbMask;

    /* Copy video bandwidth informations. */
    if (call_ptr->rtpVideoLcl.videoAsBwKbps > 0) {
        s_ptr->video.lclVideoAsBwKbps = call_ptr->rtpVideoLcl.videoAsBwKbps;
    }
    if (call_ptr->rtpVideoRmt.videoAsBwKbps > 0) {
        s_ptr->video.rmtVideoAsBwKbps = call_ptr->rtpVideoRmt.videoAsBwKbps;
    }

    /* Loop through the coders and load them into the message */

    /* Populate coders */
    OSAL_memCpy(s_ptr->coders, call_ptr->coders, sizeof(s_ptr->coders));
    ISIG_logStream(s_ptr);

    return (m_ptr);
}

/*
 * ======== ISIM_tone() ========
 * A function that allocates a ISIP_Message object and populates the object
 * with data necessary to start/stop an audio tone.  This function is
 * used to command the VTSP module to start/stop tones to the local entity.
 *
 * call_ptr : A pointer to ISID_CallId object that contains all the details
 *            regarding the call that wants to generate the tone.
 *
 * reason : A enumerated value representing "audio command".
 *          possible values are ISIP_MEDIA_REASON_TONESTART,
 *                              ISIP_MEDIA_REASON_TONESTOP,
 *                              ISIP_MEDIA_REASON_TONESTART_CMD,
 *                              ISIP_MEDIA_REASON_TONESTOP_CMD.
 *
 * tone : A enumerated value representing the tone to generate.  See
 *        the definition of ISI_AudioTone for more details.
 *
 * duration : The time in milliseconds to generate the tone.
 *
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message *ISIM_tone(
    ISID_CallId      *call_ptr,
    ISIP_MediaReason  reason,
    ISI_AudioTone     toneType,
    vint              duration)
{
    ISIP_Media   *a_ptr;
    ISIP_Message *m_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_header(m_ptr, call_ptr->e.desc, ISIP_CODE_MEDIA,
            call_ptr->service_ptr->protocol);

    a_ptr = &m_ptr->msg.media;

    a_ptr->reason = reason;
    a_ptr->media.tone.toneType = toneType;
    a_ptr->media.tone.make1 = duration;
    return (m_ptr);
}

ISIP_Message *ISIM_media(
    ISI_MediaControl mediaCmd,
    vint             arg)
{
    ISIP_Media      *a_ptr;
    ISIP_Message    *m_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }
    _ISIM_header(m_ptr, 0, ISIP_CODE_MEDIA, ISI_PROTOCOL_VTSP);
    a_ptr = &m_ptr->msg.media;

    switch (mediaCmd) {
        case ISI_MEDIA_CNTRL_SPEAKER:
            a_ptr->reason = ISIP_MEDIA_REASON_SPEAKER;
            a_ptr->media.speakerOn = (0 == arg) ? 0 : 1;
            break;
        case ISI_MEDIA_CNTRL_AEC_BYPASS:
            a_ptr->reason = ISIP_MEDIA_REASON_AEC_ENABLE;
            a_ptr->media.aecOn = (0 == arg) ? 0 : 1;
            break;
        case ISI_MEDIA_CNTRL_GAIN_CTRL:
            a_ptr->reason = ISIP_MEDIA_REASON_GAIN_CONTROL;
            a_ptr->media.gainCtrl.txGain = (arg >> 16);
            a_ptr->media.gainCtrl.rxGain = ((arg << 16) >> 16);
            break;
        case ISI_MEDIA_CNTRL_CN_GAIN_CTRL:
            a_ptr->reason = ISIP_MEDIA_REASON_CN_GAIN_CONTROL;
            a_ptr->media.gainCtrl.txGain = arg;
            break;
        default:
            ISIM_free(m_ptr);
            m_ptr = NULL;
    }
    return (m_ptr);
}

/*
 * ======== ISIM_ring() ========
 * A function that allocates a ISIP_Message object and populates the object
 * with data necessary to start/stop a ring tone.  This function is
 * used to command the VTSP module to start/stop ringing.
 *
 * call_ptr : A pointer to ISID_CallId object that contains all the details
 *            regarding the call that wants to generate the ring.
 *
 * reason : A enumerated value representing "audio command".
 *          possible values are ISIP_MEDIA_REASON_RINGSTART,
 *                              ISIP_MEDIA_REASON_RINGSTOP.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message *ISIM_ring(
    ISID_CallId      *call_ptr,
    ISIP_MediaReason  reason)
{
    ISIP_Message *m_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_header(m_ptr, call_ptr->e.desc, ISIP_CODE_MEDIA,
            call_ptr->service_ptr->protocol);

    m_ptr->msg.media.reason = reason;
    m_ptr->msg.media.media.ring.ringtemplate = call_ptr->ringTemplate;
    return m_ptr;
}

/*
 * ======== ISIM_system() ========
 * A function that allocates a ISIP_Message object and populates the object
 * with data that controls the underlying protocol's 'initialize' and
 * 'shutdown routines'.  These are commands that are on a system wide basis
 * where as other commands in this file are on a "service", "call",
 * "text message" basis.  Hence the name "ISIM_system".
 *
 * x : The protocol number.  0=VTSP, 1=SIP, 2=xmpp, 3 and greater are for
 *     other plug and play protocol definitions.
 *
 * reason : A enumerated value representing "system command" for the
 *          underlying protocol(s). Possible values are:
 *              ISIP_SYSTEM_REASON_START,
 *              ISIP_SYSTEM_REASON_SHUTDOWN,
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message *ISIM_system(
    vint               protocol,
    ISIP_SystemReason  reason,
    ISIP_Status        status)
{
    ISIP_Message *m_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    /* Id is unused for "system" commands */
    m_ptr->id = 0;
    m_ptr->code = ISIP_CODE_SYSTEM;
    m_ptr->protocol = protocol;
    m_ptr->msg.system.reason = reason;
    m_ptr->msg.system.status = status;
    m_ptr->msg.system.protocolIpc[0] = 0;
    m_ptr->msg.system.mediaIpc[0] = 0;
    return (m_ptr);
}

static void _ISIM_populateChatHeader(
    ISIP_Message        *msg_ptr,
    ISI_Id               serviceId,
    ISI_Id               chatId,
    int                  protocol,
    ISIP_ChatReason      reason,
    ISIP_Status          status)
{
    ISIP_Chat  *c_ptr;

    _ISIM_header(msg_ptr, chatId, ISIP_CODE_CHAT, protocol);

    c_ptr = &msg_ptr->msg.groupchat;
    c_ptr->reason = reason;
    c_ptr->status = status;
    c_ptr->serviceId = serviceId;
    return;
}

void ISIM_chat(
    ISIP_Message        *msg_ptr,
    ISIP_ChatReason      reason,
    ISIP_Status          status)
{
    _ISIM_populateChatHeader(msg_ptr, msg_ptr->msg.groupchat.serviceId,
            msg_ptr->id, msg_ptr->protocol, reason, status);
    return;
}

ISIP_Message *ISIM_initiateChat(
    ISID_GChatId        *chat_ptr,
    ISIP_ChatReason      reason,
    char                *to_ptr,
    char                *password_ptr,
    ISI_SessionType      type)
{
    ISIP_Chat *c_ptr;
    ISIP_Message   *m_ptr;
    
    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_populateChatHeader(m_ptr, chat_ptr->service_ptr->e.desc, chat_ptr->e.desc,
            chat_ptr->service_ptr->protocol, reason, ISIP_STATUS_TRYING);

    c_ptr = &m_ptr->msg.groupchat;

    c_ptr->type = type;
    OSAL_strncpy(c_ptr->remoteAddress, chat_ptr->szRemoteAddress, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->localAddress, chat_ptr->szLocalIdentity, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->subject, chat_ptr->szSubject, ISI_SUBJECT_STRING_SZ);
    /* Password is optional, so only populate if it exists */
    if (password_ptr && 0 != *password_ptr) {
        OSAL_strncpy(c_ptr->password, password_ptr, ISI_PASSWORD_SZ);
        c_ptr->passwordRequired = 1;
    }
    /* to which is optional */
    if (to_ptr && 0 != *to_ptr) {
        /*
         * For adhoc group chats the 'to' could be a long
         * list of comma delimited participants.
         */
        OSAL_strncpy(c_ptr->participants, to_ptr, ISI_LONG_ADDRESS_STRING_SZ);
    }

    /*
     * contributionId, which is optional, but is stored in the database and will be empty
     * and NULL terminated if unused
     */
    OSAL_strncpy(c_ptr->contributionId, chat_ptr->szContributionId, ISI_CONTRIBUTION_ID_STRING_SZ);

    return (m_ptr);
}

ISIP_Message *ISIM_inviteChat(
    ISID_GChatId     *chat_ptr,
    char             *participant_ptr,
    char             *reason_ptr)
{
    ISIP_Chat *c_ptr;
    ISIP_Message   *m_ptr;
    
    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }


    _ISIM_populateChatHeader(m_ptr, chat_ptr->service_ptr->e.desc,
            chat_ptr->e.desc, chat_ptr->service_ptr->protocol,
            ISIP_GROUP_CHAT_REASON_INVITE, ISIP_STATUS_DONE);

    c_ptr = &m_ptr->msg.groupchat;

    OSAL_strncpy(c_ptr->remoteAddress, chat_ptr->szRemoteAddress, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->localAddress, chat_ptr->szLocalIdentity, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->subject, chat_ptr->szSubject, ISI_SUBJECT_STRING_SZ);
    OSAL_strncpy(c_ptr->participants, participant_ptr, ISI_LONG_ADDRESS_STRING_SZ);
    if (reason_ptr) {
        OSAL_strncpy(c_ptr->reasonDesc, reason_ptr, ISI_EVENT_DESC_STRING_SZ);
    }
    return (m_ptr);
}

ISIP_Message *ISIM_acceptChat(
    ISID_GChatId     *chat_ptr,
    char             *password_ptr)
{
    ISIP_Chat *c_ptr;
    ISIP_Message   *m_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_populateChatHeader(m_ptr, chat_ptr->service_ptr->e.desc, chat_ptr->e.desc,
            chat_ptr->service_ptr->protocol, ISIP_CHAT_REASON_ACCEPT,
            ISIP_STATUS_DONE);

    c_ptr = &m_ptr->msg.groupchat;

    OSAL_strncpy(c_ptr->remoteAddress, chat_ptr->szRemoteAddress, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->localAddress, chat_ptr->szLocalIdentity, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->subject, chat_ptr->szSubject, ISI_SUBJECT_STRING_SZ);
    if (password_ptr) {
        OSAL_strncpy(c_ptr->password, password_ptr, ISI_PASSWORD_SZ);
    }
    return (m_ptr);
}

ISIP_Message *ISIM_rejectChat(
    ISID_GChatId     *chat_ptr,
    char             *reason_ptr)
{
    ISIP_Chat *c_ptr;
    ISIP_Message   *m_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_populateChatHeader(m_ptr, chat_ptr->service_ptr->e.desc, chat_ptr->e.desc,
            chat_ptr->service_ptr->protocol, ISIP_CHAT_REASON_REJECT,
            ISIP_STATUS_DONE);

    c_ptr = &m_ptr->msg.groupchat;

    OSAL_strncpy(c_ptr->remoteAddress, chat_ptr->szRemoteAddress, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->localAddress, chat_ptr->szLocalIdentity, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->subject, chat_ptr->szSubject, ISI_SUBJECT_STRING_SZ);
    if (reason_ptr) {
        OSAL_strncpy(c_ptr->reasonDesc, reason_ptr, ISI_EVENT_DESC_STRING_SZ);
    }
    return (m_ptr);
}

ISIP_Message *ISIM_ackChat(
    ISID_GChatId     *chat_ptr,
    char             *reason_ptr)
{
    ISIP_Chat *c_ptr;
    ISIP_Message   *m_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_populateChatHeader(m_ptr, chat_ptr->service_ptr->e.desc, chat_ptr->e.desc,
            chat_ptr->service_ptr->protocol, ISIP_CHAT_REASON_ACK,
            ISIP_STATUS_DONE);

    c_ptr = &m_ptr->msg.groupchat;

    OSAL_strncpy(c_ptr->remoteAddress, chat_ptr->szRemoteAddress, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->localAddress, chat_ptr->szLocalIdentity, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->subject, chat_ptr->szSubject, ISI_SUBJECT_STRING_SZ);
    if (reason_ptr) {
        OSAL_strncpy(c_ptr->reasonDesc, reason_ptr, ISI_EVENT_DESC_STRING_SZ);
    }
    return (m_ptr);
}

ISIP_Message *ISIM_kickChat(
    ISID_GChatId     *chat_ptr,
    char             *participant_ptr,
    char             *reason_ptr)
{
    ISIP_Chat      *c_ptr;
    ISIP_Message   *m_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_populateChatHeader(m_ptr, chat_ptr->service_ptr->e.desc, chat_ptr->e.desc,
            chat_ptr->service_ptr->protocol, ISIP_GROUP_CHAT_REASON_KICK,
            ISIP_STATUS_DONE);

    c_ptr = &m_ptr->msg.groupchat;

    OSAL_strncpy(c_ptr->remoteAddress, chat_ptr->szRemoteAddress, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->localAddress, chat_ptr->szLocalIdentity, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->subject, chat_ptr->szSubject, ISI_SUBJECT_STRING_SZ);
    OSAL_strncpy(c_ptr->participants, participant_ptr, ISI_LONG_ADDRESS_STRING_SZ);
    if (reason_ptr) {
        OSAL_strncpy(c_ptr->reasonDesc, reason_ptr, ISI_EVENT_DESC_STRING_SZ);
    }

    return (m_ptr);
}

ISIP_Message *ISIM_terminateChat(
    ISID_GChatId     *chat_ptr,
    char             *reason_ptr)
{
    ISIP_Chat      *c_ptr;
    ISIP_Message   *m_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_populateChatHeader(m_ptr, chat_ptr->service_ptr->e.desc, chat_ptr->e.desc,
            chat_ptr->service_ptr->protocol, ISIP_CHAT_REASON_TERMINATE,
            ISIP_STATUS_DONE);

    c_ptr = &m_ptr->msg.groupchat;

    OSAL_strncpy(c_ptr->remoteAddress, chat_ptr->szRemoteAddress, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->localAddress, chat_ptr->szLocalIdentity, ISI_ADDRESS_STRING_SZ);
    if (reason_ptr) {
        OSAL_strncpy(c_ptr->reasonDesc, reason_ptr, ISI_EVENT_DESC_STRING_SZ);
    }
    return (m_ptr);
}

ISIP_Message *ISIM_destroyChat(
    ISID_GChatId     *chat_ptr,
    char             *reason_ptr)
{
    ISIP_Chat      *c_ptr;
    ISIP_Message   *m_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_populateChatHeader(m_ptr, chat_ptr->service_ptr->e.desc, chat_ptr->e.desc,
            chat_ptr->service_ptr->protocol, ISIP_GROUP_CHAT_REASON_DESTROY,
            ISIP_STATUS_DONE);

    c_ptr = &m_ptr->msg.groupchat;

    OSAL_strncpy(c_ptr->remoteAddress, chat_ptr->szRemoteAddress, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(c_ptr->localAddress, chat_ptr->szLocalIdentity, ISI_ADDRESS_STRING_SZ);
    if (reason_ptr) {
        OSAL_strncpy(c_ptr->reasonDesc, reason_ptr, ISI_EVENT_DESC_STRING_SZ);
    }
    return (m_ptr);
}

/*
 * ======== ISIM_setaKaAuthResp() ========
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * result : The result of AKA response.
 *
 * resp_ptr : A NULL terminated string representing the resp
 *
 * resLength : The length of response that can vary between 32 and 128 bits.
 *
 * auts_ptr : A NULL terminated string representing the auts
 *
 * ck_ptr : A NULL terminated string representing the ck
 *
 * ik_ptr : A NULL terminated string representing the ik
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_setAkaAuthResp(
    ISID_ServiceId *service_ptr,
    int             result,
    char           *resp_ptr,
    int             resLength,
    char           *auts_ptr,
    char           *ck_ptr,
    char           *ik_ptr)
{
    ISIP_Message *msg_ptr;
    ISIP_Service *s_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_AUTH_AKA_RESPONSE, ISI_SERVER_TYPE_INVALID);

    s_ptr = &msg_ptr->msg.service;

    s_ptr->settings.akaAuthResp.result = result;
    OSAL_memCpy(s_ptr->settings.akaAuthResp.resp, resp_ptr,
            sizeof(s_ptr->settings.akaAuthResp.resp));
    s_ptr->settings.akaAuthResp.resLength = resLength;
    OSAL_memCpy(s_ptr->settings.akaAuthResp.auts, auts_ptr,
            sizeof(s_ptr->settings.akaAuthResp.auts));
    OSAL_memCpy(s_ptr->settings.akaAuthResp.ck, ck_ptr,
            sizeof(s_ptr->settings.akaAuthResp.ck));
    OSAL_memCpy(s_ptr->settings.akaAuthResp.ik, ik_ptr,
            sizeof(s_ptr->settings.akaAuthResp.ik));

    return msg_ptr;
}

/*
 * ======== ISIM_setCapabilties() ========
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * capabilities_ptr : A String that specifies the capabilties
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_setCapabilties(
    ISID_ServiceId *service_ptr,
    char           *capabilities_ptr)
{
    ISIP_Message *msg_ptr;
    ISIP_Service *s_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_CAPABILITIES, ISI_SERVER_TYPE_INVALID);
    s_ptr = &msg_ptr->msg.service;
    s_ptr->settings.capabilities[0] = 0;
    OSAL_strncpy(s_ptr->settings.capabilities, capabilities_ptr, ISI_PRESENCE_STRING_SZ);
    return msg_ptr;
}

/*
 * ======== ISIM_diagAudioRecord() ========
 * protocol : The protocol number.
 * file_ptr : The file name with full path to do the record.
 *
 * Returns: ISIP_Message object that contain the diagnostic audio 
 *     record information.
 */

ISIP_Message* ISIM_diagAudioRecord(
    vint            protocol,
    char           *file_ptr)
{
    ISIP_Message *m_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    /* Id is unused for "diagnostic" commands */
    m_ptr->id = 0;
    m_ptr->code = ISIP_CODE_DIAGNOSTIC;
    m_ptr->protocol = protocol;
    m_ptr->msg.diag.reason = ISIP_DIAG_REASON_AUDIO_RECORD;
    OSAL_strncpy(m_ptr->msg.diag.audioFile, file_ptr, ISI_ADDRESS_STRING_SZ);
    return (m_ptr);
}

/*
 * ======== ISIM_diagAudioPlay() ========
 * protocol : The protocol number.
 * file_ptr : The file name with full path to do the play.
 *
 * Returns: ISIP_Message object that contain the diagnostic audio
 *     play information.
 */
ISIP_Message* ISIM_diagAudioPlay(
    vint            protocol,
    char           *file_ptr)
{
    ISIP_Message *m_ptr;

    if ((m_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    /* Id is unused for "diagnostic" commands */
    m_ptr->id = 0;
    m_ptr->code = ISIP_CODE_DIAGNOSTIC;
    m_ptr->protocol = protocol;
    m_ptr->msg.diag.reason = ISIP_DIAG_REASON_AUDIO_PLAY;
    OSAL_strncpy(m_ptr->msg.diag.audioFile, file_ptr, ISI_ADDRESS_STRING_SZ);

    return (m_ptr);
}

/*
 * ======== ISIM_sendUssd() ========
 * A function that populates an ISIP_Message object with data necessary to send
 * a ussd message.
 *
 * ussd_ptr : A pointer to ISID_UssdId object that contains all the details
 *            regarding the ussd message to sent.

 * serviceId : A unique identifier representing the service that the ussd
 *             message will be sent within.
 *
 * chatId : A unique identifier representing the chat room that the ussd
 *          message will be sent within.  If Zero, then the ussd message is
 *          not associated with any specific grou pchat room.
 *
 * Returns:
 *  Nothing.
 */

void ISIM_sendUssd(
    ISIP_UssdReason reason,
    ISID_UssdId    *ussd_ptr,
    ISI_Id          serviceId)
{
    ISIP_Ussd    *t_ptr;
    ISIP_Message *m_ptr;

    m_ptr = &ussd_ptr->isiMsg;

    /* The id for the underlying protocol to use is the text id */
    _ISIM_header(m_ptr, ussd_ptr->e.desc, ISIP_CODE_USSD,
            ussd_ptr->service_ptr->protocol);

    t_ptr = &m_ptr->msg.ussd;
    t_ptr->reason = reason;
    t_ptr->serviceId = serviceId;
    /* The actual ussd message details have already been written */
    return;
}

/*
 * ======== ISIM_setProvisioningData() ========
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * xmlDoc_ptr: A String that specifies the provisioning data xml doc to
 *             configure.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_setProvisioningData(
    ISID_ServiceId *service_ptr,
    const char     *xmlDoc_ptr)
{
    ISIP_Message *msg_ptr;
    ISIP_Service *s_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    _ISIM_serviceHeader(service_ptr, msg_ptr,
            ISIP_SERVICE_REASON_SET_PROVISIONING_DATA,
            ISI_SERVER_TYPE_INVALID);

    s_ptr = &msg_ptr->msg.service;
    OSAL_strncpy(s_ptr->settings.provisioningData, xmlDoc_ptr,
            ISI_PROVISIONING_DATA_STRING_SZ);
    return (msg_ptr);
}

/*
 * ======== ISIM_net() ========
 * A function that allocates a ISIP_Message object and
 * populates the object with data necessary to indicate some net problem.
 *
 * service_ptr : A pointer to a ISID_ServiceId object that specifies
 *               the service in question.
 *
 * Returns:
 *  ISIP_Message* : A pointer to a newly allocated ISIP_Message object.
 */
ISIP_Message* ISIM_net(
    ISID_ServiceId *service_ptr)
{
    ISIP_Message *msg_ptr;
    ISIP_Service *s_ptr;

    if ((msg_ptr = ISIM_alloc()) == NULL) {
        return (NULL);
    }

    s_ptr = &msg_ptr->msg.service;

    _ISIM_serviceHeader(service_ptr, msg_ptr,
             ISIP_SERVICE_REASON_NET, ISI_SERVER_TYPE_INVALID);

    /* Let's put the interface info in there also. */
    OSAL_strncpy(s_ptr->settings.interface.name,
            service_ptr->szInfcName, ISI_ADDRESS_STRING_SZ);
    s_ptr->settings.interface.address = service_ptr->infcAddress;

   return msg_ptr;
}

