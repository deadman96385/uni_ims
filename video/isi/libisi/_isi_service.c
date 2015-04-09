/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 27088 $ $Date: 2014-06-21 12:13:27 +0800 (Sat, 21 Jun 2014) $
 *
 */
#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_queue.h"
#include "_isi_msg.h"
#include "_isi_service.h"
#include "_isi_dbg.h"

/* 
 * ======== ISIS_protoMsg() ========
 * This function is the FSM entry point of commands related to the service.
 * For example, service configuration stuff.  
 *
 * Currently the FSM only looks for events related to the "activation"
 * of the service.  Meaning, it only wants events that trigger the 
 * service as being in a "active" or "inactive" state.
 *
 * Returns: 
 *  nothing
 */
void ISIS_protoMsg(
    ISI_Id        serviceId, 
    ISIP_Message *msg_ptr)
{
    ISID_ServiceId    *service_ptr;
    ISIP_ServiceReason reason;
    ISIP_Status        status;
    ISI_EventMessage   event;

    reason = msg_ptr->msg.service.reason;
    status = msg_ptr->msg.service.status;
    
    event.serviceId = serviceId;
    event.id = serviceId;
    event.idType = ISI_ID_TYPE_SERVICE;
    event.event = ISI_EVENT_NONE;
    event.eventDesc[0] = 0;
    if (0 != msg_ptr->msg.service.reasonDesc[0]) {
        OSAL_strncpy(event.eventDesc, msg_ptr->msg.service.reasonDesc,
                ISI_EVENT_DESC_STRING_SZ);
    }

    ISIG_log(__FUNCTION__, 0, 0, 0 ,0);
    ISIG_logService(serviceId, reason);

    if (ISID_lockServices() != ISI_RETURN_OK) {
        return;
    }

    /* Get the pointer to the internal service object */
    if (ISID_serviceGet(serviceId, &service_ptr) == ISI_RETURN_FAILED) {
        /* This service must be non-existent so just return */
        ISID_unlockServices();
        return;
    }

    switch (reason) {
    case ISIP_SERVICE_REASON_AUTH:
        if (status == ISIP_STATUS_FAILED) {
            event.event = ISI_EVENT_CREDENTIALS_REJECTED;
        }
        else {
            /* Set the state to activated */
            service_ptr->isActivated = 1;
            event.event = ISI_EVENT_SERVICE_ACTIVE;
        }
        break;
    case ISIP_SERVICE_REASON_NET:
        if (status == ISIP_STATUS_FAILED) {
            event.event = ISI_EVENT_NET_UNAVAILABLE;
        }
        break;
    case ISIP_SERVICE_REASON_CREATE:
        if (status == ISIP_STATUS_DONE) {
            event.event = ISI_EVENT_SERVICE_INIT_OK;
        }
        else {
            event.event = ISI_EVENT_SERVICE_INIT_FAILED;
            ISID_serviceDestroy(serviceId);
        }
        break;
    case ISIP_SERVICE_REASON_ACTIVATE:
        /* Set the state to activated */
        if (status == ISIP_STATUS_TRYING) {
            event.event = ISI_EVENT_SERVICE_ACTIVATING;
        }
        else {
            service_ptr->isActivated = 1;
            event.event = ISI_EVENT_SERVICE_ACTIVE;
        }
        break;
    case ISIP_SERVICE_REASON_DEACTIVATE:
        /* Set the state to deactivated */
        service_ptr->isActivated = 0;
        event.event = ISI_EVENT_SERVICE_INACTIVE;
        break;
    case ISIP_SERVICE_REASON_HANDOFF:
        event.event = ISI_EVENT_SERVICE_HANDOFF;
        break;
    case ISIP_SERVICE_REASON_BLOCKUSER:
    case ISIP_SERVICE_REASON_IDENTITY:
    case ISIP_SERVICE_REASON_SERVER:
    case ISIP_SERVICE_REASON_CODERS:
    case ISIP_SERVICE_REASON_URI:
    case ISIP_SERVICE_REASON_INSTANCEID:
    case ISIP_SERVICE_REASON_FILE:
    case ISIP_SERVICE_REASON_PORT:
        break;
    case ISIP_SERVICE_REASON_IPSEC:
        service_ptr->portUc =
                msg_ptr->msg.service.settings.ipsec.resp.portUc;
        service_ptr->portUs =
                msg_ptr->msg.service.settings.ipsec.resp.portUs;
        service_ptr->portPc =
                msg_ptr->msg.service.settings.ipsec.resp.portPc;
        service_ptr->portPs =
                msg_ptr->msg.service.settings.ipsec.resp.portPs;
        service_ptr->spiUc =
                msg_ptr->msg.service.settings.ipsec.resp.spiUc;
        service_ptr->spiUs =
                msg_ptr->msg.service.settings.ipsec.resp.spiUs;
        service_ptr->spiPc =
                msg_ptr->msg.service.settings.ipsec.resp.spiPc;
        service_ptr->spiPs =
                msg_ptr->msg.service.settings.ipsec.resp.spiPs;
        if (status == ISIP_STATUS_INVALID) {
            event.event = ISI_EVENT_IPSEC_RELEASE;
        }
        else {
            event.event = ISI_EVENT_IPSEC_SETUP;
        }
        break;
    case ISIP_SERVICE_REASON_AUTH_AKA_CHALLENGE:
        /* AKA challenge */
        OSAL_memCpy(service_ptr->rand,
                msg_ptr->msg.service.settings.akaAuthChallenge.rand,
                sizeof(service_ptr->rand));
        OSAL_memCpy(service_ptr->autn,
                msg_ptr->msg.service.settings.akaAuthChallenge.autn,
                sizeof(service_ptr->autn));
        event.event = ISI_EVENT_AKA_AUTH_REQUIRED;
        break;
    case ISIP_SERVICE_REASON_LAST:
    default:
        break;
    }
    if (event.event != ISI_EVENT_NONE) {
        /* Then send it to the app queue */
        ISIQ_writeAppQueue((char*)&event, sizeof(ISI_EventMessage));
    }
    /* **************************************
     * NOTE: You don't have to free msg_ptr, 
     * because it came from .bss and not heap
     **************************************** 
     */
    ISID_unlockServices();
}

/* 
 * ======== ISIS_appMsg() ========
 * This function is the FSM entry point of commands related to a 'service'.
 * These commands come as a result of an API call.
 *
 * Currently this function contains no true FSM, it simply passes
 * the commands downstream to the underlying protocol.
 *
 * Returns: 
 *   ISI_RETURN_OK : always..
 */
ISI_Return ISIS_appMsg(
    ISIP_Message *msg_ptr)
{
    ISIP_ServiceReason reason;
    
    reason = msg_ptr->msg.service.reason;
    
    ISIG_log(__FUNCTION__, 0, 0, 0 ,0);
    ISIG_logService(msg_ptr->id, reason);

    /* 
     * ADD any cases for particular service reasons
     * and special handling if required 
     */
    switch (reason) {
    case ISIP_SERVICE_REASON_AUTH:
    case ISIP_SERVICE_REASON_NET:
    case ISIP_SERVICE_REASON_INVALID:
    case ISIP_SERVICE_REASON_CREATE:
    case ISIP_SERVICE_REASON_DESTROY:
    case ISIP_SERVICE_REASON_ACTIVATE:
    case ISIP_SERVICE_REASON_DEACTIVATE:
    case ISIP_SERVICE_REASON_HANDOFF:
    case ISIP_SERVICE_REASON_BLOCKUSER:
    case ISIP_SERVICE_REASON_IDENTITY:
    case ISIP_SERVICE_REASON_SERVER:
    case ISIP_SERVICE_REASON_CODERS:
    case ISIP_SERVICE_REASON_URI:
    case ISIP_SERVICE_REASON_INSTANCEID:
    case ISIP_SERVICE_REASON_FILE:
    case ISIP_SERVICE_REASON_AUTH_AKA_CHALLENGE:
    case ISIP_SERVICE_REASON_AUTH_AKA_RESPONSE:
    case ISIP_SERVICE_REASON_CAPABILITIES:
    case ISIP_SERVICE_REASON_PORT:
    case ISIP_SERVICE_REASON_IPSEC:
    case ISIP_SERVICE_REASON_LAST:
    default:
        break;
    }
    
    ISIQ_writeProtocolQueue(msg_ptr);
    ISIM_free(msg_ptr);
    return (ISI_RETURN_OK);
}

