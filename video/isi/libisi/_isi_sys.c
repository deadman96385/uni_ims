/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 23082 $ $Date: 2013-11-21 16:17:55 +0800 (Thu, 21 Nov 2013) $
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
 * ======== ISIY_protoMsg() ========
 * This function handles events coming from the underlying protocol
 * Currently nothing is defined to be coming from the underlying protocol
 * Therefore, any events are quietly discarded.
 *
 * Returns: 
 *  nothing
 */
void ISIY_protoMsg(
    ISIP_Message *msg_ptr)
{
    
    ISI_EventMessage   event;

    event.serviceId = 0;
    event.id        = msg_ptr->protocol;
    event.idType    = ISI_ID_TYPE_NONE;
    event.event     = ISI_EVENT_NONE;
    event.eventDesc[0] = 0;

    /* ISIG_log(__FUNCTION__, 0, 0, 0 ,0);
    ISIG_logSystem(msg_ptr->msg.system.reason); */

    if (msg_ptr->msg.system.reason == ISIP_SYSTEM_REASON_START) {
        /* Then a protocol has indicated that it has successfully started */
        
        /* 
         * Check whether or not if we already know about this protocol. Only 
         * tell the application about this if the "READY" event is new. We
         * do not want to send duplicate "READY" events. 
         */
        if (ISI_RETURN_OK != ISID_checkProtocol(msg_ptr->protocol)) {
            if (ISI_PROTOCOL_VTSP != msg_ptr->protocol) {
                /* Tell the application about the initialization of this protocol */
                event.event = ISI_EVENT_PROTOCOL_READY;
            }
        }
        else {
            /* This protocol already added. */
            if ((ISIP_STATUS_INVALID == msg_ptr->msg.system.status) &&
                    (ISI_PROTOCOL_VTSP != msg_ptr->protocol)) {
                /*
                 * The protocol restarted abnormally.
                 * Process it as failed.
                 */
                ISIG_log(__FUNCTION__, 0, 0, 0 ,0);
                ISIG_logSystem(msg_ptr->msg.system.reason);

                /*
                 * Then protocol here has indicated that it is supported BUT it has 
                 * FAILED to start/initialize.
                 */
                ISID_setProtocol(ISID_PROTOCOLS_FAILED, msg_ptr->protocol, 1);
                /* Disable it so the user application can't use it */
                ISID_setProtocol(ISID_PROTOCOLS_ACTIVE, msg_ptr->protocol, 0);
                ISIQ_clearProtocol(msg_ptr->protocol);
                /* Then the ptotocol app shutdown. Tell the application */
                event.event = ISI_EVENT_PROTOCOL_FAILED; 
                ISIG_log(__FUNCTION__,
                        "Protocol abnormal restarted.\n"
                        "ISI_EVENT_PROTOCOL_FAILED event generated for protocol:%d", 
                        event.id, 0, 0);
                ISIQ_writeAppQueue((char*)&event, sizeof(ISI_EventMessage));
                /* Generate a READY event then */
                event.event = ISI_EVENT_PROTOCOL_READY;
            }
        }
            
        /* Set the protocol as active and cache it's IPC information */
        ISID_setProtocol(ISID_PROTOCOLS_ACTIVE, msg_ptr->protocol, 1);
        ISIQ_addProtocol(msg_ptr->protocol, msg_ptr->msg.system.protocolIpc,
                msg_ptr->msg.system.mediaIpc, msg_ptr->msg.system.streamIpc);
        /* 
         * Return a "Done" to notify the protocol application that we recv'd
         * the registration.
         */
        msg_ptr->msg.system.status = ISIP_STATUS_DONE;
        ISIQ_writeProtocolQueue(msg_ptr);
    }
    else if (msg_ptr->msg.system.reason == ISIP_SYSTEM_REASON_SHUTDOWN) {
        
        ISIG_log(__FUNCTION__, 0, 0, 0 ,0);
        ISIG_logSystem(msg_ptr->msg.system.reason);

        /*
         * Then protocol here has indicated that it is supported BUT it has 
         * FAILED to start/initialize.
         */
        ISID_setProtocol(ISID_PROTOCOLS_FAILED, msg_ptr->protocol, 1);
        /* Disable it so the user application can't use it */
        ISID_setProtocol(ISID_PROTOCOLS_ACTIVE, msg_ptr->protocol, 0);
        ISIQ_clearProtocol(msg_ptr->protocol);
        /* Then the ptotocol app shutdown. Tell the application */
        if (ISI_PROTOCOL_VTSP != msg_ptr->protocol) {
            event.event = ISI_EVENT_PROTOCOL_FAILED; 
        }
    }
    
    /* 
     * Now update the "requestProtocol" bitmask, so we can track who we have
     * responses from 
     */
    
/* 
 * XXX Enable the following code for backwards compatiblity with old 'vapp' 
 * Eventually this code can be removed after pre-alpha release.
 */    
#if 0
    if (ISID_setProtocol(ISID_PROTOCOLS_REQUESTED,
            msg_ptr->protocol, 0) == 0) {
        /* 
         * Then we have a response from all requested protocols,
         * tell the app 
         */
        if (ISID_getProtocol(ISID_PROTOCOLS_FAILED) != 0) {
            /* Then there was some failure somewhere, tell the app */
            event.event = ISI_EVENT_PROTOCOL_FAILED; 
        }
        else {
            /* Tell the app of out success */
            event.event = ISI_EVENT_PROTOCOL_READY;
        }
    }
#endif
    
    if (event.event != ISI_EVENT_NONE) {
        /* Then send it to the app queue */
        ISIG_log(__FUNCTION__,
               "ISI_EVENT_PROTOCOL_READY event generated for protocol:%d", 
               event.id, 0, 0);
        ISIQ_writeAppQueue((char*)&event, sizeof(ISI_EventMessage));
    }
}

/* 
 * ======== ISIY_appMsg() ========
 * This function is the FSM entry point of commands related to 'system'
 * commands. These commands come as a result of an API call.
 *
 * Currently this function contains no true FSM, it simply passes
 * the commands downstream to the underlying protocol.
 *
 * Returns: 
 *   ISI_RETURN_OK : always.
 */
ISI_Return ISIY_appMsg(
    ISIP_Message *msg_ptr)
{
    ISIG_log(__FUNCTION__, 0, 0, 0 ,0);
    ISIG_logSystem(msg_ptr->msg.system.reason);

    /* ADD any special handling if required */
    
    ISIQ_writeProtocolQueue(msg_ptr);
    ISIM_free(msg_ptr);
    return (ISI_RETURN_OK);
}

