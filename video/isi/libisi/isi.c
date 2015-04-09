/* THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 *
 */
#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_queue.h"
#include "_isi_msg.h"
#include "_isi_service.h"
#include "_isi_call.h"
#include "_isi_gchat.h"
#include "_isi_text.h"
#include "_isi_file.h"
#include "_isi_evt.h"
#include "_isi_conf.h"
#include "_isi_pres.h"
#include "_isi_sys.h"
#include "_isi_media.h"
#include "_isi_dbg.h"
#include "_isi_diag.h"
#include "_isi_ussd.h"
#include "_isi_xml.h"
#include "_isi_mem.h"

/*
 * ISI release string.
 */
extern char const D2_Release_ISI[];

/* Global buffer  for ISI_getEvent(). */
static char *_buf_ptr = NULL;

/*
 * ======== ISI_allocate() ========
 * This function must be called before calling any another ISI API function.
 * A call to this function allocate the ISI layer internals. Applications
 * can not call any other ISI API function prior to calling this function.
 *
 * country_ptr : A pointer to an object containing info regarding the
 *               country to support.
 *
 * Return Values:
 * ISI_RETURN_OK                : The ISI layer was properly initialized.
 * ISI_RETURN_INVALID_COUNTRY   : Initialization failed due to an unknown or
 *                                unsupported country type.
 * ISI_RETURN_MUTEX_ERROR       : ISI Initialization failed.  There was an error
 *                                with creating internally used mutexes.
 * ISI_RETURN_FAILED            : The ISI layer could not be initialized. The
 *                                IPC Pipe mechanisms could not be initialized
 *                                or the ISI module has already been init'd.
 */
ISI_Return ISI_allocate(
    char   *country_ptr)
{
    /* Allocate memory for global buffer. */
    if (NULL == (_buf_ptr = OSAL_memAlloc(sizeof(ISIP_Message) + 1,
            OSAL_MEM_ARG_STATIC_ALLOC))) {
        return (ISI_RETURN_FAILED);
    }

    if (ISID_getState()) {
        /* then we are currently active */
        return (ISI_RETURN_FAILED);
    }
    else {
#ifndef ISI_DYNAMIC_MEMORY_ALLOC
        /* Initial memory pool for memory alloc */
        if (ISI_RETURN_OK != ISIMP_memPoolInit()) {
             return (ISI_RETURN_NO_MEM);
        }
#endif
        /* Init all internally used objects in the data sub-module*/
        if (ISID_init() != ISI_RETURN_OK) {
            /* Then there was a problem initializing the internal mutexes */
            return (ISI_RETURN_MUTEX_ERROR);
        }

        /* Init the queues used for the application & protocol msgs */
        if (ISIQ_init() != ISI_RETURN_OK) {
            /* Then there was a problem with the underlying queue init */
            return (ISI_RETURN_FAILED);
        }

        /* Set the country */
        ISID_setCountry(country_ptr);

        /* Set the state as 'active' or 'ready' */
        ISID_setState(1);
        return (ISI_RETURN_OK);
    }
}

/*
 * ======== ISI_init() ========
 * This function must be called before calling any another ISI API function.
 * A call to this function initializes the ISI layer internals. Applications
 * can not call any other ISI API function prior to calling this function.
 *
 * country_ptr : A pointer to an object containing info regarding the
 *               country to support.
 *
 * Return Values:
 * ISI_RETURN_OK                : The ISI layer was properly initialized.
 * ISI_RETURN_INVALID_COUNTRY   : Initialization failed due to an unknown or
 *                                unsupported country type.
 * ISI_RETURN_MUTEX_ERROR       : ISI Initialization failed.  There was an error
 *                                with creating internally used mutexes.
 * ISI_RETURN_FAILED            : The ISI layer could not be initialized. The
 *                                IPC Pipe mechanisms could not be initialized
 *                                or the ISI module has already been init'd.
 */
ISI_Return ISI_init(
    char   *country_ptr)
{
    /* Allocate memory for global buffer. */
    if (NULL == (_buf_ptr = OSAL_memAlloc(sizeof(ISIP_Message) + 1,
            OSAL_MEM_ARG_STATIC_ALLOC))) {
        return (ISI_RETURN_FAILED);
    }

    if (ISID_getState()) {
        /* then we are currently active */
        return (ISI_RETURN_FAILED);
    }
    else {
#ifndef ISI_DYNAMIC_MEMORY_ALLOC
        /* Initial memory pool for memory alloc */
        if (ISI_RETURN_OK != ISIMP_memPoolInit()) {
             return (ISI_RETURN_NO_MEM);
        }
#endif
        /* Init all internally used objects in the data sub-module*/
        if (ISID_init() != ISI_RETURN_OK) {
            /* Then there was a problem initializing the internal mutexes */
            return (ISI_RETURN_MUTEX_ERROR);
        }

        /* Init the queues used for the application & protocol msgs */
        if (ISIQ_init() != ISI_RETURN_OK) {
            /* Then there was a problem with the underlying queue init */
            return (ISI_RETURN_FAILED);
        }

        /* Set the country */
        ISID_setCountry(country_ptr);

        /* Set the state as 'active' or 'ready' */
        ISID_setState(1);
        return (ISI_RETURN_OK);
    }
}

/*
 * ======== ISI_getVersion() ========
 * This function is used to retrieve the software version number of ISI module.
 *
 * version_ptr :  A pointer to a buffer where ISI will write the version
 *                number. This buffer must be ISI_VERSION_STRING_SZ bytes in
 *                length.
 *
 * Return Values:
 * ISI_RETURN_OK        : Success
 * ISI_RETURN_NOT_INIT  : ISI not initialized.
 */
ISI_Return ISI_getVersion(
    char *version_ptr)
{

    if (!ISID_getState()) {
        /* then we are not currently active */
        return (ISI_RETURN_NOT_INIT);
    }

    OSAL_strncpy(version_ptr, D2_Release_ISI, ISI_VERSION_STRING_SZ);

    return (ISI_RETURN_OK);
}

static void _ISI_releaseAllServiceTransactions(ISI_Id serviceId)
{
    ISID_ServiceId *service_ptr;
    ISIP_Message   *msg_ptr;
    ISID_TextId    *text_ptr;
    ISID_FileId    *file_ptr;
    ISID_PresId    *pres_ptr;
    ISID_GChatId   *chat_ptr;
    ISID_CallId    *call_ptr;

    /*
     * Ensure that there's no 'left-over' transactions in any
     * of transaction lists
     */
    if (ISI_RETURN_OK == ISID_lockPres()) {
        if (ISI_RETURN_FAILED != ISID_serviceGet(serviceId, &service_ptr)) {
            while(ISI_RETURN_OK == ISID_presGetFirstByServiceId(service_ptr, &pres_ptr)) {
                ISID_presDestroy(pres_ptr->e.desc);
            }
        }
        ISID_unlockPres();
    }
    if (ISI_RETURN_OK == ISID_lockTexts()) {
        if (ISI_RETURN_FAILED != ISID_serviceGet(serviceId, &service_ptr)) {
            while(ISI_RETURN_OK == ISID_textGetFirstByServiceId(service_ptr, &text_ptr)) {
                ISID_textDestroy(text_ptr->e.desc);
            }
        }
        ISID_unlockTexts();
    }
    if (ISI_RETURN_OK == ISID_lockFiles()) {
        if (ISI_RETURN_FAILED != ISID_serviceGet(serviceId, &service_ptr)) {
            while(ISI_RETURN_OK == ISID_fileGetFirstByServiceId(service_ptr, &file_ptr)) {
                ISID_fileDestroy(file_ptr->e.desc);
            }
        }
        ISID_unlockFiles();
    }
    if (ISI_RETURN_OK == ISID_lockGChats()) {
        if (ISI_RETURN_FAILED != ISID_serviceGet(serviceId, &service_ptr)) {
            while(ISI_RETURN_OK == ISID_gchatGetFirstByServiceId(service_ptr, &chat_ptr)) {
                /* Now notify the protocol */
                if (NULL != (msg_ptr = ISIM_terminateChat(chat_ptr, ""))) {
                    ISIG_appMsg(chat_ptr, msg_ptr);
                }
                ISID_gchatDestroy(chat_ptr->e.desc);
            }
        }
        ISID_unlockGChats();
    }
    if (ISI_RETURN_OK == ISID_lockCalls()) {
        if (ISI_RETURN_FAILED != ISID_serviceGet(serviceId, &service_ptr)) {
            while(ISI_RETURN_OK == ISID_callGetFirstByServiceId(service_ptr, &call_ptr)) {
                /* Now notify the protocol */
                if (NULL != (msg_ptr = ISIM_updateCall(call_ptr,
                        ISIP_CALL_REASON_TERMINATE, ISIP_STATUS_INVALID, NULL, NULL))) {
                    ISIC_appMsg(call_ptr, msg_ptr);
                }
                if (0 != call_ptr->e.desc) {
                    ISID_callDestroy(call_ptr->e.desc);
                }
                else {
                    ISID_callDestroy(call_ptr->e.desc2);
                }
            }
        }
        ISID_unlockCalls();
    }
    return;
}

/*
 * ======== ISI_shutdown() ========
 * This function is called to release all resources owned by the ISI layer.
 * All states will be lost and all active sessions will be dropped gracefully.
 * This is equivalent to software reset of the ISI layer.  After the ISI layer
 * is shut down, ISI_init() must be called before calling any other ISI
 * function. If the ISI layer is shut down properly, the function returns
 * ISI_RETURN_OK.  If ISI layer is already shut down, or fails to shut down,
 * the function returns ISI_RETURN_FAILED.
 *
 * Return Value:
 * ISI_RETURN_OK        : ISI shutdown successful.
 * ISI_RETURN_FAILED    : ISI shutdown failed.
 * ISI_RETURN_NOT_INIT  : ISI not initialized.
 */
ISI_Return ISI_shutdown(void)
{
    ISID_ServiceId *service_ptr;
    ISIP_Message   *msg_ptr;
    vint            x;
    
    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Now deactivate services and free them */
    while (ISID_serviceGetFirst(&service_ptr) == ISI_RETURN_OK) {
        ISI_deactivateService(service_ptr->e.desc);
        ISI_freeService(service_ptr->e.desc);
    }

    /* Set the state to NOT active (or '0') */
    ISID_setState(0);

    /*
     * Wake up the 'select' in the ISI_getEvent() just in case the calling
     * application is still trying to call ISI_getEvent() this will let them
     * know the ISI module is being destroyed.
     */
    ISIQ_wakeAppQueue();
    /* Give the thread that may be performing ISI_getEvent() a chance to die */
    OSAL_taskDelay(500);

    /*
     * Issue commands to the underlying protocols
     * to stop audio, xmpp, and sip resources, etc, "plug and play"
     * protocols/modules.
     * But first destroy proto queue so that ISI does not get anything from
     * protocols in response to shutdown cmd.
     */
    x = 0;
    ISIQ_destroyApp();
    while (x < ISID_MAX_NUM_PROTOCOLS) {
        if (ISID_checkProtocol(x) == ISI_RETURN_OK) {
            msg_ptr = ISIM_system(x, ISIP_SYSTEM_REASON_SHUTDOWN,
                    ISIP_STATUS_DONE);
            if (msg_ptr) {
                ISIY_appMsg(msg_ptr);
            }
        }
        x++;
    }
    ISIQ_destroyProto();
    ISIQ_destroyGroupQ();

    /* Free the mutexes use in the db */
    ISID_destroy();
#ifndef ISI_DYNAMIC_MEMORY_ALLOC    
    /* Free all the memory */
    ISIMP_memPoolDestroy();
#endif
    /* Free global buffer. */
    if (NULL != _buf_ptr) {
        OSAL_memFree(_buf_ptr, OSAL_MEM_ARG_STATIC_ALLOC);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_allocService() ========
 * This function is called to allocate a service using ISI.  A service
 * facilitates establishment of sessions between endpoints directly or through
 * a server. Several services can exist on a system at one time.
 *
 * serviceId_ptr : a pointer to a unique identification number for the service
 *                 allocated.  This unique ID is returned by ISI layer to the
 *                 application for future references involving the allocated
 *                 service.
 *
 * protocol : A numerical value that represents the type of signaling
 *            protocol to be used for this service. (i.e. SIP, xmpp etc).
 *            valid values are 0 = reserved for internal use. 1 = SIP,
 *            2 = XMPP, 3 or more...Is for "plug and play" protocols.
 *            (or application defined protocols).
 *
 * Return Values:
 * ISI_RETURN_OK                : Service allocation successful.
 * ISI_RETURN_INVALID_PROTOCOL  : Unsupported signalling protocol.
 * ISI_RETURN_NOT_INIT          : ISI layer not Initialized
 * ISI_RETURN_FAILED            : Could not allocate service.
 *                                There are no available resources.
 * ISI_RETURN_MUTEX_ERROR       : Could not lock the Service linked list.
 */
ISI_Return ISI_allocService(
    ISI_Id            *serviceId_ptr,
    int                protocol)
{
    ISID_ServiceId *service_ptr;
    ISIP_Message   *msg_ptr;
    ISI_Id          serviceId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!serviceId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /*
     * Check that the protocol is allowed.  Remember to disallow the
     * protocol for 'vtsp' as well.
     */
    if (protocol ==  ISI_PROTOCOL_VTSP ||
            ISID_checkProtocol(protocol) != ISI_RETURN_OK) {
        return (ISI_RETURN_INVALID_PROTOCOL);
    }

    service_ptr = ISID_serviceCreate();
    if (service_ptr == NULL) {
        return (ISI_RETURN_FAILED);
    }
    /* set your attributes */
    service_ptr->protocol = protocol;

    /* Take a mutex against the services database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        ISI_free(service_ptr, ISI_OBJECT_SERVICE_ID);
        return (ISI_RETURN_MUTEX_ERROR);
    }
    serviceId = 0;
    if (ISID_serviceAdd(service_ptr, &serviceId) != ISI_RETURN_OK) {
        ISI_free(service_ptr, ISI_OBJECT_SERVICE_ID);
        ISID_unlockServices();
        return (ISI_RETURN_FAILED);
    }
    /* Notify the protocol layer that the service is being built */
    if ((msg_ptr = ISIM_service(service_ptr, 1)) == NULL) {
        ISID_serviceDestroy(service_ptr->e.desc);
        ISID_unlockServices();
        return (ISI_RETURN_FAILED);
    }
    /* Send the message to the protocol */
    ISIS_appMsg(msg_ptr);
    *serviceId_ptr = serviceId;
    ISID_unlockServices();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_activateService() ========
 * This function is called to activate a service using ISI.  A service must have
 * been previously allocated via ISI_allocService(). A call to this function
 * initiates service activation. Applications calling this function must not
 * consider the service as activated until a ISI_EVENT_SERVICE_ACTIVE event is
 * generated to the application.  If service activation fails, ISI will generate
 * a ISI_EVENT_SERVICE_INACTIVE event.

 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * Return Values:
 * ISI_RETURN_OK                     : Service activation successful.
 * ISI_RETURN_FAILED                 : Service activation failed.
 * ISI_RETURN_SERVICE_ALREADY_ACTIVE : Service exists.
 * ISI_RETURN_INVALID_SERVICE_ID     : Service ID is invalid.
 * ISI_RETURN_NOT_INIT               : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR            : Could not lock the Service linked list.
 */
ISI_Return ISI_activateService(
    ISI_Id serviceId)
{
    ISID_ServiceId *service_ptr;
    ISI_Return      ret;
    ISIP_Message   *msg_ptr;
    
    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }
    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    ret = ISID_serviceGet(serviceId, &service_ptr);
    if (ret == ISI_RETURN_FAILED) {
        /* Then this service does not even exist */
        ISID_unlockServices();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }
    else if (ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
        /* If we are here then the 'ret' is ISI_RETURN_SERVICE_NOT_ACTIVE
         * which means that the service was valid AND
         * currently deactivated. So continue.
         */
        if ((msg_ptr = ISIM_activation(service_ptr, 1)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    /* If we are here, then the serviceId is valid but it's active! */
    ISID_unlockServices();
    return (ISI_RETURN_SERVICE_ALREADY_ACTIVE);
}

/*
 * ======== ISI_deactivateService() ========
 * This function is called to deactivate a service that has been previously
 * activated via ISI_activateService(). If a service is successfully deactivated
 * A call to this function initiates service deactivation.  A service is not
 * deactivated until ISI generates ISI_EVENT_SERVICE_INACTIVE event to the
 * application. If service deactivation fails a ISI_EVENT_SERVICE_ACTIVE event
 * is generated to the application.
 *
 * serviceId : serviceId, is the service identification number returned by
 *             ISI when ISI_allocService() was called.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Service deactivation successful.
 * ISI_RETURN_FAILED             : Service deactivation failed
 * ISI_RETURN_SERVICE_BUSY       : Service is busy.
 * ISI_RETURN_SERVICE_NOT_ACTIVE : Service is inactive.
 * ISI_RETURN_INVALID_SERVICE_ID : Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_deactivateService(
    ISI_Id serviceId)
{
    ISID_ServiceId *service_ptr;
    ISI_Return      ret;
    ISIP_Message   *msg_ptr;
    
    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    ret = ISID_serviceGet(serviceId, &service_ptr);
    if (ret == ISI_RETURN_FAILED) {
        /* Then this service does not even exist */
        ISID_unlockServices();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }
    else if (ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
        /*
         * If we are here then the 'ret' is ISI_RETURN_SERVICE_NOT_ACTIVE
         * which means that the service was valid BUT not active
         */
        ISID_unlockServices();
        return (ISI_RETURN_SERVICE_NOT_ACTIVE);
    }
    else {
        /* All is good. Now tell the protocol layer */
        if ((msg_ptr = ISIM_activation(service_ptr, 0)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        service_ptr->isActivated = 0;
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
}

/*
 * ======== ISI_freeService() ========
 * This function is called to free a service that has been previously allocated
 * via ISI_allocService().  If the service is active, it must be first be
 * deactivated via ISI_deactivateService(). It is the responsibility of the
 * application to free an allocated service when it is no longer needed.
 * Leftover services waste memory.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Service successfully destroyed.
 * ISI_RETURN_INVALID_SERVICE_ID : Service ID is invalid.
 * ISI_RETURN_FAILED             : Service destruction failed.
 * ISI_RETURN_NOT_INIT           : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_freeService(
    ISI_Id serviceId)
{
    ISID_ServiceId *service_ptr;
    ISI_Return       ret;
    ISIP_Message    *msg_ptr;
    
    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }
    
    _ISI_releaseAllServiceTransactions(serviceId);

    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    ret = ISID_serviceGet(serviceId, &service_ptr);
    if (ret == ISI_RETURN_FAILED) {
        /* Then this service does not even exist */
        ISID_unlockServices();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }
    else if (ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
        /*
         * If we are here then the 'ret' is ISI_RETURN_SERVICE_NOT_ACTIVE
         * which means that the service was valid AND
         * currently deactivated, so free it and return (ISI_RETURN_OK)
         */
        /* Notify the protocol layer that the service is being destroyed */
        if ((msg_ptr = ISIM_service(service_ptr, 0)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_serviceDestroy(serviceId);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }

    /* If we are here, then the serviceId is valid but it's active!
     * The user must deactivate first, so let them know.
     */
    ISID_unlockServices();
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISI_serviceMakeCidPrivate() ========
 * This function is used to enable or disable including the caller
 * identification when placing calls.  For this feature to work, the underlying
 * service protocol must support privacy mechanisms to hide or show caller
 * identification.  The Service does not to be active before this function call.
 *
 * serviceId : This is the service identification number returned by ISI
 *             when ISI_allocService() was called.
 *
 * isPrivate : Enables or disables caller identification send.  if > 1 then
 *             caller ID will not be sent when a call is established. If == 0,
 *             caller identification will be sent when a call is established.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Service caller ID send state changed.
 * ISI_RETURN_INVALID_SERVICE_ID : Service ID is invalid.
 * ISI_RETURN_FAILED             : Could not notify the protocol
 * ISI_RETURN_NOT_INIT           : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceMakeCidPrivate(
    ISI_Id serviceId,
    int    isPrivate)
{
    ISID_ServiceId *service_ptr;
    ISIP_Message    *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid.
         * It may or not be 'activated' but we don't care
         */

        /* Let the protocol know */
        if ((msg_ptr = ISIM_privateCid(service_ptr, isPrivate)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        /* Force privateCid value to strictly be either '0' or '1' */
        service_ptr->privateCid = (isPrivate) ? 1 : 0;
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_serviceSetBsid() ========
 * This function is used to set or modify the BSID associated with the service
 * specified in serviceId. The Service need not be active before this function
 * call.

 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * type : Network access type.
 *
 * bsid_ptr : A null terminated string containing the new BSID.
 *
 * Return Values:
 * ISI_RETURN_OK                 : The BSID was set for the service.
 * ISI_RETURN_INVALID_SERVICE_ID : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI is not initialized
 * ISI_RETURN_FAILED             : Could not notify the service protocol.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceSetBsid (
    ISI_Id                 serviceId,
    ISI_NetworkAccessType  type,
    char                  *bsid_ptr)
{
    ISID_ServiceId *service_ptr;
    ISIP_Message   *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!bsid_ptr || *bsid_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid.
         * It may or not be 'activated' but we don't care.
         */
        service_ptr->bsId.type = type;
        OSAL_strncpy(service_ptr->bsId.szBsId, bsid_ptr,
                ISI_BSID_STRING_SZ);
        if ((msg_ptr = ISIM_bsid(service_ptr)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_serviceSetImei() ========
 * This function is used to set or modify the IMEI which is for anonymous 
 * emergency call.
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * imei_ptr : A null terminated string containing the new IMEI.
 *
 * Return Values:
 * ISI_RETURN_OK                 : The IMEI was set for the service.
 * ISI_RETURN_INVALID_SERVICE_ID : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI is not initialized
 * ISI_RETURN_FAILED             : Could not notify the service protocol.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceSetImeiUri(
    ISI_Id  serviceId,
    char   *imei_ptr)
{
    ISID_ServiceId *service_ptr;
    ISIP_Message   *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!imei_ptr || *imei_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid.
         * It may or not be 'activated' but we don't care.
         */
        OSAL_strncpy(service_ptr->imeiUri, imei_ptr, ISI_ADDRESS_STRING_SZ);
        if ((msg_ptr = ISIM_imei(service_ptr)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_serviceSetInterface() ========
 * This function is used to set or modify the radio interface to use for the
 * service specified in serviceId. The Service need not be active before this
 * routine is called.

 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * name_ptr : A null terminated string containing the name of the interface to use.
 *
 * address_ptr : A null termianted string containing the IP address (IPv4 or IPv6)
 * of the interface to use.
 *
 * Return Values:
 * ISI_RETURN_OK                 : The radio interface was set for the service.
 * ISI_RETURN_INVALID_SERVICE_ID : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI is not initialized
 * ISI_RETURN_FAILED             : Could not convert the IP address specified in address_ptr.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceSetInterface(
    ISI_Id  serviceId,
    char   *name_ptr,
    char   *address_ptr)
{
    ISID_ServiceId *service_ptr;
    OSAL_NetAddress host_address;
    ISIP_Message   *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!name_ptr || *name_ptr == 0 || !address_ptr || *address_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid.
         * It may or not be 'activated' but we don't care.
         */
        if (OSAL_FAIL == OSAL_netStringToAddress((int8 *)address_ptr, 
                    &host_address)) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        /* Need to record address in host byte format. Convert. */
        OSAL_netAddrNtoh(&service_ptr->infcAddress, &host_address);
        OSAL_strncpy(service_ptr->szInfcName, name_ptr, ISI_ADDRESS_STRING_SZ);
        /* Generate ISIP message to sapp. */
        if ((msg_ptr = ISIM_net(service_ptr)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_serviceSetUri() ========
 * This function is used to set or modify the URI associated with the service
 * specified in serviceId. The Service need not be active before this function
 * call.

 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * uri_ptr : A null terminated string containing the new URI.
 *
 * Return Values:
 * ISI_RETURN_OK                 : The URI was set for the service.
 * ISI_RETURN_INVALID_SERVICE_ID : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI is not initialized
 * ISI_RETURN_FAILED             : Could not notify the service protocol.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceSetUri(
    ISI_Id  serviceId,
    char   *uri_ptr)
{
    ISID_ServiceId *service_ptr;
    ISIP_Message   *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!uri_ptr || *uri_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid.
         * It may or not be 'activated' but we don't care.
         */
        OSAL_strncpy(service_ptr->szUri, uri_ptr, ISI_ADDRESS_STRING_SZ);
        if ((msg_ptr = ISIM_uri(service_ptr)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_serviceSetInstanceId() ========
 * This function is used to set or modify the Instance id of UA associated
 * with the service specified in serviceId.

 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * instance_ptr : A null terminated string containing the device Id of UA.
 *
 * Return Values:
 * ISI_RETURN_OK                 : The URI was set for the service.
 * ISI_RETURN_INVALID_SERVICE_ID : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI is not initialized
 * ISI_RETURN_FAILED             : Could not notify the service protocol.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceSetInstanceId(
    ISI_Id  serviceId,
    char   *instance_ptr)
{
    ISID_ServiceId *service_ptr;
    ISIP_Message   *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!instance_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid.
         * It may or not be 'activated' but we don't care.
         */
        OSAL_strncpy(service_ptr->szInstanceId, instance_ptr,
                ISI_ADDRESS_STRING_SZ);
        if ((msg_ptr = ISIM_instanceId(service_ptr)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_serviceSetCapabilities() ========
 * Applications call this function to update capabilities information of this
 * entity.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called
 *
 * capabilities_ptr : A null terminated string containing the protocol specific
 *             capabilities string.  This string represents the entity's actual
 *             state of capabilities. Typically the format of the string is XML.
 *
 * Return Values:
 * ISI_RETURN_OK            : The capabilities info was successfully sent.
 * ISI_RETURN_FAILED        : Function Failed. The capabilities information
 *                            could not be sent.
 * ISI_RETURN_INVALID_SERVICE_ID: Service ID is not valid.
 * ISI_RETURN_NOT_INIT      : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR   : Could not lock the capabilities linked list.
 */
ISI_Return ISI_serviceSetCapabilities(
    ISI_Id  serviceId,
    char   *capabilities_ptr)
{
    ISID_ServiceId *service_ptr;
    ISIP_Message   *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!capabilities_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid.
         * It may or not be 'activated' but we don't care.
         */
        if ((msg_ptr = ISIM_setCapabilties(service_ptr, capabilities_ptr)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_serviceSetIpsec() ========
 * This function is used to set or modify IPSec information with the service
 * specified in serviceId. The Service need not be active before this function
 * call.

 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * port       : The protected port.
 * portPoolSz : The pool size of protected ports.
 * spi        : SPI of SA.
 * spiPoolSz  : The pool size of SPI.
 *
 * Return Values:
 * ISI_RETURN_OK                 : IPSec info was set for the service.
 * ISI_RETURN_INVALID_SERVICE_ID : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI is not initialized.
 * ISI_RETURN_FAILED             : Could not notify the service protocol.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceSetIpsec(
    ISI_Id  serviceId,
    int     port,
    int     portPoolSz,
    int     spi,
    int     spiPoolSz)
{
    ISID_ServiceId *service_ptr;
    ISIP_Message   *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid.
         * It may or not be 'activated' but we don't care.
         */
        service_ptr->protectedPort       = port;
        service_ptr->protectedPortPoolSz = portPoolSz;
        service_ptr->spi                 = spi;
        service_ptr->spiPoolSz           = spiPoolSz;

        if ((msg_ptr = ISIM_ipsec(service_ptr)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_serviceSetServer() ========
 * This function is used to set or modify any servers associated with the
 * service specified in serviceId. The Service need not be active before
 * calling this function.  If a server is not set with a service then
 * that service will bypass all facilities provided by that server.
 * For example, if STUN server is not set, the service will assume open IP
 * and if proxy server is not set for a SIP service, peer to peer calling is
 * used.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * server_ptr : A null terminated string containing the new server address.
 *              The address must be a FQDN or an IP address in dotted decimal
 *              notation.
 *
 * type : The type of server to be set.  Types of servers supported by ISI and
 *        its underlying protocol and call control layers are enumerated in
 *        ISI_ServerType.
 *
 * Return Values:
 * ISI_RETURN_OK                  : Server set properly for service.
 * ISI_RETURN_INVALID_SERVER_TYPE : Server type is not recognized.
 * ISI_RETURN_INVALID_SERVICE_ID  : Service ID is invalid.
 * ISI_RETURN_NOT_INIT            : ISI not initialized
 * ISI_RETURN_FAILED              : Could not notify the protocol
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceSetServer(
    ISI_Id          serviceId,
    char           *server_ptr,
    ISI_ServerType  type)
{
    ISID_ServiceId  *service_ptr;
    char            *str_ptr;
    ISI_Return       ret;
    ISIP_Message    *msg_ptr;
    int              length;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!server_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid. It may or not be 'activated' but we
         * don't care.
         */
        switch (type) {
        case ISI_SERVER_TYPE_STUN:
            ret = ISI_RETURN_OK;
            str_ptr = service_ptr->szStunServer;
            length = sizeof(service_ptr->szStunServer);
            break;
        case ISI_SERVER_TYPE_PROXY:
            ret = ISI_RETURN_OK;
            str_ptr = service_ptr->szProxy;
            length = sizeof(service_ptr->szProxy);
            break;
        case ISI_SERVER_TYPE_REGISTRAR:
            ret = ISI_RETURN_OK;
            str_ptr = service_ptr->szRegistrar;
            length = sizeof(service_ptr->szRegistrar);
            break;
        case ISI_SERVER_TYPE_OUTBOUND_PROXY:
            ret = ISI_RETURN_OK;
            str_ptr = service_ptr->szOutboundProxy;
            length = sizeof(service_ptr->szOutboundProxy);
            break;
        case ISI_SERVER_TYPE_RELAY:
            ret = ISI_RETURN_OK;
            str_ptr = service_ptr->szRelayServer;
            length = sizeof(service_ptr->szRelayServer);
            break;
        case ISI_SERVER_TYPE_STORAGE:
            ret = ISI_RETURN_OK;
            str_ptr = service_ptr->szStorageServer;
            length = sizeof(service_ptr->szStorageServer);
            break;
        case ISI_SERVER_TYPE_CHAT:
            ret = ISI_RETURN_OK;
            str_ptr = service_ptr->szChatServer;
            length = sizeof(service_ptr->szChatServer);
            break;
        default:
            ISID_unlockServices();
            return (ISI_RETURN_INVALID_SERVER_TYPE);
        }
        if (ret == ISI_RETURN_OK) {
            OSAL_strncpy(str_ptr, server_ptr, length);
            if ((msg_ptr = ISIM_server(service_ptr, type)) == NULL) {
                ISID_unlockServices();
                return (ISI_RETURN_FAILED);
            }
            ISIS_appMsg(msg_ptr);
            ISID_unlockServices();
            return (ISI_RETURN_OK);
        }
        ISID_unlockServices();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_serviceSetPort() ========
 * This function is used to set or modify sip, audio or video ports associated
 * with the service specified in serviceId.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * port : The port number to be set to the specific port type
 *
 * poolSize = The port pool size to be set of the specific port type
 *
 * type : The type of port to be set.
 *
 * Return Values:
 * ISI_RETURN_OK                  : Server set properly for service.
 * ISI_RETURN_INVALID_SERVER_TYPE : Server type is not recognized.
 * ISI_RETURN_INVALID_SERVICE_ID  : Service ID is invalid.
 * ISI_RETURN_NOT_INIT            : ISI not initialized
 * ISI_RETURN_FAILED              : Could not notify the protocol
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceSetPort(
    ISI_Id          serviceId,
    int             port,
    int             poolSize,
    ISI_PortType    type)
{
    ISID_ServiceId  *service_ptr;
    ISIP_Message     *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
   if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid. It may or not be 'activated' but we
         * don't care.
         */
        switch (type) {
        case ISI_PORT_TYPE_SIP:
            service_ptr->sipPort = port;
            break;
        case ISI_PORT_TYPE_AUDIO:
            service_ptr->audioRtpPort = port;
            service_ptr->audioPoolSize = poolSize;
            break;
        case ISI_PORT_TYPE_VIDEO:
            service_ptr->videoRtpPort = port;
            service_ptr->videoPoolSize = poolSize;
            break;
        default:
            ISID_unlockServices();
            return (ISI_RETURN_INVALID_SERVER_TYPE);
        }
        if ((msg_ptr = ISIM_port(service_ptr, type)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_serviceSetFilePath() ========
 * This function is used to set or modify a path that is used when downloading
 * files for this service.  This path is used to place downloaded files into
 * the file system.  Additionally, a string that will be prepended to all
 * downloaded files can be set via this routine.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * filePath_ptr : A null terminated string containing the absolute path to use
 *              when writing downloaded files to the file system.
 *
 * filePrepend_ptr : A null terminated string containing a string to
 *                   prepend to all downloaded files.
 *
 * Return Values:
 * ISI_RETURN_OK                  : the file path and prepend string was set
 *                                  for service.
 * ISI_RETURN_INVALID_SERVICE_ID  : Service ID is invalid.
 * ISI_RETURN_NOT_INIT            : ISI not initialized
 * ISI_RETURN_FAILED              : Could not notify the protocol
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceSetFilePath(
    ISI_Id          serviceId,
    char           *filePath_ptr,
    char           *filePrepend_ptr)
{
    ISID_ServiceId  *service_ptr;
    ISIP_Message     *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        if ((msg_ptr = ISIM_filePath(service_ptr, filePath_ptr,
                filePrepend_ptr)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}


/*
 * ======== ISI_serviceSetCredentials() ========
 * This function is used to set or change account credentials related to a
 * service.  Credentials may be required for authenticating with the service
 * provider.  The Service need not be active before this function call. To keep
 * several sets of username and password pairs, user can make several calls to
 * this function with different realm values.
 * To delete a username and password pair for a particular realm, the user must
 * supply zero length (NULL terminated) strings for username and password but
 * still supply a valid realm. If no username/password pair exists for a service
 * then authentication mechanisms are disabled for that service.
 *
 * serviceId : This is the service identification number returned by ISI when
 *            ISI_allocService() was called.
 *
 * username_ptr : A null terminated string containing the username of the
 *                account.
 *
 * password_ptr : A null terminated string containing the password for the
 *                account.
 *
 * realm_ptr : A null terminated string containing realm for the account.
 *
 * Return Values:
 * ISI_RETURN_OK                  : Credentials assigned/changed successfully.
 * ISI_RETURN_INVALID_SERVICE_ID  : Service ID is invalid.
 * ISI_RETURN_INVALID_CREDENTIALS : Credentials are not supplied properly.
 * ISI_RETURN_NOT_INIT            : ISI not initialized
 * ISI_RETURN_FAILED              : Could not notify the protocol
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the Service linked list.
 *
 */
ISI_Return ISI_serviceSetCredentials(
    ISI_Id  serviceId,
    char   *username_ptr,
    char   *password_ptr,
    char   *realm_ptr)
{
    ISID_ServiceId *service_ptr;
    ISID_Account   *acct_ptr;
    ISIP_Message    *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /*
     * Verify all parameters are NOT NULL. Also, in all cases of using this
     * function, 'realm' is mandatory however it may be *realm_ptr == 0.
     */
    if (!realm_ptr || !username_ptr || !password_ptr) {
        return (ISI_RETURN_INVALID_CREDENTIALS);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Check the serviceId it may or may not be active.  But we don't care.*/
    if (ISID_serviceGet(serviceId, &service_ptr) == ISI_RETURN_FAILED) {
        /* Then the service does not exist */
        ISID_unlockServices();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }


    if (*username_ptr == 0 && *password_ptr == 0) {
        /* Then the caller wants ISI to remove the credentials */
        if (ISID_accountRemove(service_ptr, realm_ptr) == ISI_RETURN_FAILED) {
            /* Then the credentials don't even exist */
            ISID_unlockServices();
            return (ISI_RETURN_INVALID_CREDENTIALS);
        }
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }

    /* If we are here then the user wants to add/modify a set of credentials */

    /* Username is mandatory so return failure if they didn't supply it */
    if (*username_ptr == 0) {
        ISID_unlockServices();
        return (ISI_RETURN_INVALID_CREDENTIALS);
    }

    /*
     * If the 'realm' already exists then the user wants to update an
     * existing entry, otherwise we will add a new entry
     */
    acct_ptr = ISID_accountFind(service_ptr, realm_ptr, 1);
    if (acct_ptr) {
        /* Found it, let's 'update' the changes */
        OSAL_strncpy(acct_ptr->szUsername, username_ptr, ISI_ADDRESS_STRING_SZ);

        /* Password is 'optional' */
        if (*password_ptr == 0) {
            acct_ptr->szPassword[0] = 0;
        }
        else {
            OSAL_strncpy(acct_ptr->szPassword, password_ptr,
                    ISI_ADDRESS_STRING_SZ);
        }
    }
    else {
        /* account does not exist so add it */
        acct_ptr = ISI_alloc(sizeof(ISID_Account), ISI_OBJECT_ACCOUNT);
        if (!acct_ptr) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        else {
            ISID_accountClean(acct_ptr);
            OSAL_strncpy(acct_ptr->szRealm, realm_ptr, ISI_ADDRESS_STRING_SZ);
            OSAL_strncpy(acct_ptr->szUsername, username_ptr,
                    ISI_ADDRESS_STRING_SZ);
            /* The password is 'optional' */
            if (*password_ptr == 0) {
                acct_ptr->szPassword[0] = 0;
            }
            else {
                OSAL_strncpy(acct_ptr->szPassword, password_ptr,
                        ISI_ADDRESS_STRING_SZ);
            }
            if (ISID_accountAdd(service_ptr, acct_ptr) == ISI_RETURN_FAILED) {
                /* Then we've reached the max number of accounts */
                ISI_free(acct_ptr, ISI_OBJECT_ACCOUNT);
                ISID_unlockServices();
                return (ISI_RETURN_FAILED);
            }
        }
    }

    /* Now notify the protocol */
    if ((msg_ptr = ISIM_credentials(service_ptr, acct_ptr)) == NULL) {
        ISID_unlockServices();
        return (ISI_RETURN_FAILED);
    }
    ISIS_appMsg(msg_ptr);
    ISID_unlockServices();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_serviceSetEmergency() ========
 * This function is used to set or emergency with the service
 * specified in serviceId. The Service need not be active before this function
 * call.

 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * uri_ptr : A null terminated string containing the new URI.
 *
 * Return Values:
 * ISI_RETURN_OK                 : The URI was set for the service.
 * ISI_RETURN_INVALID_SERVICE_ID : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI is not initialized
 * ISI_RETURN_FAILED             : Could not notify the service protocol.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceSetEmergency(
    ISI_Id  serviceId,
    int     isEmergency)
{
    ISID_ServiceId *service_ptr;
    ISIP_Message   *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid.
         * It may or not be 'activated' but we don't care.
         */
        service_ptr->isEmergency = isEmergency;
        if ((msg_ptr = ISIM_emergency(service_ptr)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}


/*
 * ======== ISI_serviceBlockUser() ========
 * This function is used to block communication from a specific user.  The
 * Service must be active before this function call. For example, if a user is
 * blocked via this function then any call attempts from that user will be
 * declined.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * username_ptr : A null terminated string containing the username to block.
 *
 * Return Values:
 * ISI_RETURN_OK                 : User blocked successfully.
 * ISI_RETURN_FAILED             : User cannot be blocked or could not notify
 *                                 the protocol
 * ISI_RETURN_INVALID_SERVICE_ID : Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceBlockUser(
    ISI_Id  serviceId,
    char   *username_ptr)
{
    ISI_Return      ret;
    ISID_ServiceId *service_ptr;
    ISIP_Message   *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!username_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    ret = ISID_serviceGet(serviceId, &service_ptr);
    if (ret == ISI_RETURN_OK) {
        /* Then the id is valid and the service is activated */
        if ((msg_ptr = ISIM_block(service_ptr, username_ptr, 1)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    else if (ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
        /* Then the id is valid but the service is deactivated */
        ISID_unlockServices();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_serviceUnblockUser() ========
 * This function is used to unblock/allow communication from a specific user.
 * The Service must be active before this function call. By default,
 * communication with all users is allowed.
 *
 * serviceId : This is the service identification number returned by ISI when
 *            ISI_allocService() was called.
 *
 * username_ptr : A null terminated string containing the username to allow.
 *
 * Return Values:
 * ISI_RETURN_OK                 : User unblocked successfully.
 * ISI_RETURN_FAILED             : User cannot be unblocked.
 * ISI_RETURN_INVALID_SERVICE_ID : Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceUnblockUser(
    ISI_Id  serviceId,
    char   *username_ptr)
{
    ISI_Return       ret;
    ISID_ServiceId *service_ptr;
    ISIP_Message   *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!username_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    ret = ISID_serviceGet(serviceId, &service_ptr);
    if (ret == ISI_RETURN_OK) {
        /* Then the id is valid and the service is activated */
        if ((msg_ptr = ISIM_block(service_ptr, username_ptr, 0)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    else if (ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
        /* Then the id is valid but the service is deactivated */
        ISID_unlockServices();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}


/*
 * ======== ISI_addCoderToService() ========
 * This function is used to add or change a coder used for communication with
 * a service.  The Service need not be active before function calling this
 * function. If the coder is already present in the service, it will be updated
 * with new coder rate and priority. Updated coder settings are used when a new
 * call is initiated or accepted.  Calls inherit service properties including
 * coder types when started.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * coderName_ptr : A null terminated string containing the name of coder.
 *
 * coderDescription_ptr : A null terminated string with coder description like
 *                        rate, priority, sdp-number etc.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Coder added/changed successfully.
 * ISI_RETURN_FAILED             : Could not issue command to set coder.
 * ISI_RETURN_INVALID_CODER      : Coder not found.
 * ISI_RETURN_INVALID_SERVICE_ID : Service ID is invalid.
 * ISI_RETURN_NOT_SUPPORTED      : Coder not supported by this service.
 * ISI_RETURN_NOT_INIT           : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_addCoderToService(
    ISI_Id  serviceId,
    char   *coderName_ptr,
    char   *coderDescription_ptr)
{
    ISIP_Message    *msg_ptr;
    ISID_ServiceId  *service_ptr;
    ISIP_Coder      *cdr_ptr;
    ISID_CoderE     *e_ptr;
    ISIP_Coder       coder;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!coderName_ptr) {
        /* We must have the coder name */
        return (ISI_RETURN_INVALID_CODER);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /*
     * Check if the service is valid. It may or may not be 'activated' but we
     * don't care.
     */
    if (ISID_serviceGet(serviceId, &service_ptr) == ISI_RETURN_FAILED) {
        /* Then the service does not even exist */
        ISID_unlockServices();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    /* First see if the coder exits */
    cdr_ptr = ISID_coderFind(service_ptr->coders, ISI_CODER_NUM,
            coderName_ptr, coderDescription_ptr);
    if (cdr_ptr) {
        /* Found it so update it */
        if (coderDescription_ptr) {
            OSAL_strncpy(cdr_ptr->description, coderDescription_ptr,
                    sizeof(cdr_ptr->description));
            /* Now notify the protocol */
            if ((msg_ptr = ISIM_coders(service_ptr)) == NULL) {
                ISID_unlockServices();
                return (ISI_RETURN_FAILED);
            }
            ISIS_appMsg(msg_ptr);
        }
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    else {
        /* Does not currently exist. Check coder against supported ISI types */
        e_ptr = ISID_coderFindIsi(coderName_ptr);
        if (e_ptr) {
            /* populate */
            OSAL_strncpy(coder.szCoderName, coderName_ptr,
                    sizeof(coder.szCoderName));
            if (coderDescription_ptr) {
                OSAL_strncpy(coder.description, coderDescription_ptr,
                        sizeof(coder.description));
            }
            coder.relates = e_ptr->relates;
            if (ISID_coderAdd(service_ptr->coders, ISI_CODER_NUM,
                    &coder) == ISI_RETURN_OK) {
                /* Now notify the protocol */
                if ((msg_ptr = ISIM_coders(service_ptr)) == NULL) {
                    ISID_unlockServices();
                    return (ISI_RETURN_FAILED);
                }
                ISIS_appMsg(msg_ptr);
                ISID_unlockServices();
                return (ISI_RETURN_OK);
            }
            ISID_unlockServices();
            return (ISI_RETURN_NOT_SUPPORTED);
        }
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_CODER);
}

/*
 * ======== ISI_removeCoderFromService() ========
 * This function is used to remove a coder used for communication with a
 * service.  The Service need not be active before calling this function.
 * Updated coder settings are used when a new call is initiated or accepted.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * codeName_ptr : A null terminated string containing the name of coder to
 *                remove.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Coder removed successfully.
 * ISI_RETURN_INVALID_CODER      : Coder not found.
 * ISI_RETURN_INVALID_SERVICE_ID : Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_removeCoderFromService(
    ISI_Id  serviceId,
    char   *coderName_ptr)
{
    ISIP_Message    *msg_ptr;
    ISID_ServiceId *service_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!coderName_ptr) {
        /* We must have the coder name */
        return (ISI_RETURN_INVALID_CODER);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid. It may or not be 'activated' but we
         * don't care.
         */
        if (ISID_coderRemove(service_ptr->coders, ISI_CODER_NUM,
                coderName_ptr) == ISI_RETURN_FAILED) {
            /* Then the coder did not exist in the service */
            ISID_unlockServices();
            return (ISI_RETURN_INVALID_CODER);
        }
        /* Now notify the protocol */
        if ((msg_ptr = ISIM_coders(service_ptr)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_removeCoderFromServiceByPayloadType() ========
 * This function is used to remove a coder used for communication with a
 * service.  The Service need not be active before calling this function.
 * Updated coder settings are used when a new call is initiated or accepted.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * codeName_ptr : A null terminated string containing the name of coder to
 *                remove.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Coder removed successfully.
 * ISI_RETURN_INVALID_CODER      : Coder not found.
 * ISI_RETURN_INVALID_SERVICE_ID : Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_removeCoderFromServiceByPayloadType(
    ISI_Id  serviceId,
    int     payloadType)
{
    ISIP_Message    *msg_ptr;
    ISID_ServiceId  *service_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid. It may or not be 'activated' but we
         * don't care.
         */
        if (ISID_coderRemoveByPayloadType(service_ptr->coders, ISI_CODER_NUM,
                payloadType) == ISI_RETURN_FAILED) {
            /* Then the coder did not exist in the service */
            ISID_unlockServices();
            return (ISI_RETURN_INVALID_CODER);
        }
        /* Now notify the protocol */
        if ((msg_ptr = ISIM_coders(service_ptr)) == NULL) {
            ISID_unlockServices();
            return (ISI_RETURN_FAILED);
        }
        ISIS_appMsg(msg_ptr);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_serviceForwardCalls() ========
 * This function is used to set conditional call forwarding for inbound calls
 * for a particular service.  The service MUST be activated before calling
 * this function.  If the function is successful then a unique identifier
 * that represents this event transaction will be written to evtId_ptr.
 *
 * evtId_ptr : A pointer to a ISI_Id.  If this function is successful then
 *             a unique identifier is written here.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * condition : An enumerated value representing which call forwarding condition
 *             to being set.  Possible values are:
 *             ISI_FORWARD_UNCONDITIONAL, ISI_FORWARD_BUSY, ISI_FORWARD_NO_REPLY
 *             ISI_FORWARD_UNREACHABLE, ISI_FORWARD_INVALID.
 *
 * enable : A value representing whether the call forwarding condition is being
 *          enabled or disabled.  1 = enable the call forwarding condition.
 *          0 = disable the call forwarding condition.
 *
 * to_ptr : A NULL terminated string representing the "forward target" (a.k.a.
 *          the address to forward to).  When 'enable' is '1', this parameter
 *          is mandatory.  When 'enable' is '0', then this parameter is ignored.
 *
 * timeout : This value is only applicable when 'condition' is
 *           'ISI_FORWARD_NO_REPLY'.  It represents the timeout in seconds
 *           before the call is forwarded.  For all other forwarding conditions
 *           this parameter will be ignored.
 *
 * Return Values:
 * ISI_RETURN_OK                 : The call forwarding command has been
 *                                 successfully issued.
 * ISI_RETURN_FAILED             : Call forwarding could not be set.  Invalid
 *                                 parameter or the service is not active.
 * ISI_RETURN_INVALID_SERVICE_ID : Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceForwardCalls(
    ISI_Id      *evtId_ptr,
    ISI_Id       serviceId,
    ISI_FwdCond  condition,
    int          enable,
    char        *to_ptr,
    int          timeout)
{
    ISI_Return      ret;
    ISID_ServiceId *service_ptr;
    ISID_EvtId     *evt_ptr;
    ISI_Id          evtId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!evtId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Verify the conditions are valid */
    if ((condition < ISI_FORWARD_UNCONDITIONAL) ||
            (condition >= ISI_FORWARD_INVALID)) {
        /* Invalid condition, return error */
        return (ISI_RETURN_FAILED);
    }

    /*
     * If we are enabling call forwarding then the forward target (to_ptr) is
     * mandatory.
     */
    if (0 != enable && NULL == to_ptr) {
        /* Then we can't enable, there's no target forwarding address */
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of events */
    if (ISID_lockEvts() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    ret = ISID_serviceGet(serviceId, &service_ptr);
    if (ret == ISI_RETURN_FAILED) {
        /* Then the service does not even exist */
        ISID_unlockEvts();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    if (ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
        /* Then the service ID is valid but the service is deactivated */
        ISID_unlockEvts();
        return (ISI_RETURN_FAILED);
    }

    /* If we are here then the service is active */
    if ((evt_ptr = ISID_evtCreate()) == NULL) {
        ISID_unlockEvts();
        return (ISI_RETURN_FAILED);
    }
    evt_ptr->service_ptr = service_ptr;

    /*
     * Before you add this object, make sure that evtId is zero.
     * This will force the DB sub module to assign an id
     */
    evtId = 0;
    if (ISID_evtAdd(evt_ptr, &evtId) == ISI_RETURN_FAILED) {
        /* Free the memory and return */
        ISI_free(evt_ptr, ISI_OBJECT_EVT_ID);
        ISID_unlockEvts();
        return (ISI_RETURN_FAILED);
    }

    /* Populate a ISIP Message and notify the protocol */
    ISIM_forward(evt_ptr, condition, enable, to_ptr, timeout);
    ISIE_appMsg(evt_ptr, &evt_ptr->isiMsg);
    *evtId_ptr = evtId;
    ISID_unlockEvts();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_initiateCall() ========
 * This function is used to initiate a call to a remote agent using a service
 * specified by serviceId.  ISI will generate the status of initiated call to
 * application using ISI events. Calls inherit default call properties from
 * the service used to initiate calls.  For example, if certain coders and
 * Caller ID blocking was enabled then those service settings are taken into
 * account when placing this call.
 *
 * callId_ptr : A pointer to a unique identification number for the call
 *              that is initiated.  This unique ID is returned by ISI layer
 *              to user application for future references involving the call.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * to_ptr : A null terminated string containing the address of the remote agent
 *          to call.
 *
 * subject_ptr : A null terminated string containing 'subject' of call.
 *               If subject_ptr is set to null, no subject will be used.
 *
 * cidType :  Describes the Caller Id policy, enumerated in ISI_SessionCidType.
 *
 * mediaAttribute_ptr : A null terminated string in xml format. This string
 *          represents the changes to be done at the media level to the call
 *          session. Please refer to _isi_xml.h for more details on various
 *          tags and attributes used.
 * e.g -
 * <media>
 *     <audio enabled="true"
 *         direction="sendrecv"
 *         secure="false"
 *         useRtpAVPF="false"
 *         maxBandwidth="0"/>
 *     <video enabled="true"
 *         direction="sendrecv"
 *         secure="false"
 *         useRtpAVPF="true"
 *         maxBandwidth="0"/>
 * </media>
 *
 * Return Values:
 * ISI_RETURN_OK                   : Call initiation successful.
 * ISI_RETURN_FAILED               : Call initiation failed.
 * ISI_RETURN_INVALID_SERVICE_ID   : Service ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the Call linked list.
 */
ISI_Return ISI_initiateCall(
    ISI_Id               *callId_ptr,
    ISI_Id                serviceId,
    char                 *to_ptr,
    char                 *subject_ptr,
    ISI_INP ISI_SessionCidType cidType,
    ISI_INP char         *mediaAttribute_ptr)
{
    ISID_CallId     *call_ptr;
    ISID_ServiceId  *service_ptr;
    ISIP_Message     *msg_ptr;
    ISI_Id            callId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!callId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Validate the to_ptr */
    if (!to_ptr || *to_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid, we don't care if the service is
         * activated.  This is needed for VCC.
         */
        if ((call_ptr = ISID_callCreateFromXml(service_ptr, to_ptr, subject_ptr,
                cidType,mediaAttribute_ptr)) == NULL) {
            ISID_unlockCalls();
            return (ISI_RETURN_FAILED);
        }
        else {
            /* Add it to the database.  Init the callId pointer to zero
             * This indicates to the DB that we wish for the DB to determine
             * the CallId.
             */
            callId = 0;
            if (ISID_callAdd(call_ptr, &callId) == ISI_RETURN_FAILED) {
                /* Free the memory and return */
                ISI_free(call_ptr, ISI_OBJECT_CALL_ID);
                ISID_unlockCalls();
                return (ISI_RETURN_FAILED);
            }
            /* Now notify the protocol */
            if (ISIM_initiateCall(service_ptr, call_ptr,
                    &msg_ptr) == ISI_RETURN_INVALID_ADDRESS) {
                /* Then there was a problem with the to_ptr field */
                ISID_callDestroy(callId);
                ISID_unlockCalls();
                return (ISI_RETURN_FAILED);
            }
            
            if (ISIC_appMsg(call_ptr, msg_ptr) == ISI_RETURN_FAILED) {
                ISID_callDestroy(callId);
                ISID_unlockCalls();
                return (ISI_RETURN_FAILED);
            }
            *callId_ptr = callId;
            ISID_unlockCalls();
            return (ISI_RETURN_OK);
        }
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_sendUSSD() ========
 * This function is used to send USSD message to GSM newtork
 * USSD services use the existing architecture of GSM networks.
 * A user dialing a USSD service code initiates a dialog with a USSD
 * application residing in a mobile network.
 *
 * to_ptr : A null terminated string containing the command to send 
 * 
*/
ISI_Return ISI_sendUSSD(
    ISI_Id          *evtId_ptr,
    ISI_Id           serviceId,
    char            *ussd_ptr)
{
    ISI_Return      ret;
    ISID_ServiceId *service_ptr;
    ISID_EvtId     *evt_ptr;
    ISI_Id          evtId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!evtId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Validate the ussd_ptr */
    if (!ussd_ptr || *ussd_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of events */
    if (ISID_lockEvts() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    OSAL_logMsg("%s : serviceId= %d \n", __FUNCTION__, serviceId);

    ret = ISID_serviceGet(serviceId, &service_ptr);
    if (ret == ISI_RETURN_FAILED) {
        OSAL_logMsg("%s : FAIL do serviceGet , id=%d\n", __FUNCTION__, serviceId);
        /* Then the service does not even exist */
        ISID_unlockEvts();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    /*
    if(ISID_serviceGetByProtocol(ISI_PROTOCOL_GSM, &service_ptr) != ISI_RETURN_FAILED) {
        OSAL_logMsg("%s : get gsm servicePTR \n", __FUNCTION__ );
    }else{
        OSAL_logMsg("%s : fai to get gsm service PTR = \n", __FUNCTION__ );
    }
    */

    if (ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
        /* Then the service ID is valid but the service is deactivated */
        ISID_unlockEvts();
        return (ISI_RETURN_FAILED);
    }

    /* If we are here then the service is active */
    if ((evt_ptr = ISID_evtCreate()) == NULL) {
        ISID_unlockEvts();
        return (ISI_RETURN_FAILED);
    }
    evt_ptr->service_ptr = service_ptr;

    /*
     * Before you add this object, make sure that evtId is zero.
     * This will force the DB sub module to assign an id
     */
    evtId = 0;
    if (ISID_evtAdd(evt_ptr, &evtId) == ISI_RETURN_FAILED) {
        /* Free the memory and return */
        ISI_free(evt_ptr, ISI_OBJECT_EVT_ID);
        ISID_unlockEvts();
        return (ISI_RETURN_FAILED);
    }

    /* Populate a ISIP Message and notify the protocol */
    ISIM_sendUSSD(evt_ptr, ussd_ptr);
    ISIE_appMsg(evt_ptr, &evt_ptr->isiMsg);
    *evtId_ptr = evtId;
    ISID_unlockEvts();
    return (ISI_RETURN_OK);

}

/*
 * ======== ISI_terminateCall() ========
 * This function is used to terminate a call in progress.  Applications do not
 * need to call this function if a call is terminated by the remote endpoint.
 * ISI will generate an event to the application to indicate the status of
 * terminated call.  After a call is terminated, all resources associated with
 * the call are freed and the callId will no longer be valid. Subsequent
 * references to a terminated call will return ISI_RETURN_INVALID_CALL_ID.

 * callId : The identification number for the call to be terminated.  This
 *          ID number was provided by ISI to the application when it
 *          initiated a call or delivered to the application through ISI
 *          event when a call was received.
 * reason_ptr : The reason description for call termination. This string
 *          may be included in SIP header for some service provider.
 *
 * Return Values:
 * ISI_RETURN_OK              : Call termination successful.
 * ISI_RETURN_FAILED          : Call termination failed.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_terminateCall(
    ISI_Id callId, 
    char  *reason_ptr)
{
    ISID_CallId *call_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_TERMINATE,
             ISIP_STATUS_INVALID, NULL, NULL)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    /* Add reason description */
    if( NULL != reason_ptr) {
        OSAL_snprintf(msg_ptr->msg.call.reasonDesc, ISI_EVENT_DESC_STRING_SZ, 
            "%s", reason_ptr);
    }
    else {
        msg_ptr->msg.call.reasonDesc[0] = '\0';
    }
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_getCallHandoff() ========
 * This function is used to get the remote address associated to a call that
 * needs to be "handed off" to another service.  This is typically needed for
 * VCC capability in FMC (Fixed Mobile Convergence) where a call on one wireless
 * network can be handed off to another wireless network. When a
 * ISI_EVENT_CALL_HANDOFF event is received by the application, the application
 * can call this function to determine the target address to attempt to contact
 * to handoff the call.
 *
 * callId : This is the call identification number of the call that is being
 *          requested to handoff.
 *
 * to_ptr: A pointer to buffer that this function will write a NULL terminated
 *         string representing the address of the remote entity to hand the call
 *         off to.
 *
 * Return Value:
 * ISI_RETURN_OK              : The function was successful. the 'to_ptr' will
 *                              be populated with a NULL terminated string
 *                              containing the address to contact to handoff
 *                              the call.
 * ISI_RETURN_FAILED          : The function was not successful.  There is no
 *                              remote address that can be used to handoff the
 *                              call.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI is not initialized
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_getCallHandoff(
    ISI_Id           callId,
    char            *to_ptr)
{
    ISID_CallId *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!to_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid. */
        if (call_ptr->szTargetUri[0] == 0) {
            ISID_unlockCalls();
            return (ISI_RETURN_FAILED);
        }
        OSAL_strncpy(to_ptr, call_ptr->szTargetUri, ISI_ADDRESS_STRING_SZ);
        ISID_unlockCalls();
        return (ISI_RETURN_OK);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}

/*
 * ======== ISI_modifyCall() ========
 * This function is used to modify a call in progress. ISI will then generate an
 * event to the application indicating the status of modified call.

 * callId : The identification number for the call to be modified.  This
 *          ID number was provided by ISI to the application when it
 *          initiated a call or delivered to the application through ISI
 *          event when a call was received..
 *
 * reason_ptr : The reason description for call rejection.
 *
 * Returns Values:
 * ISI_RETURN_OK              : Call modified successfully.
 * ISI_RETURN_FAILED          : Call modified failed.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_modifyCall(
    ISI_Id callId,
    char   *reason_ptr)
{
    ISID_CallId *call_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }

    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_MODIFY,
            ISIP_STATUS_INVALID, NULL, NULL)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }

    /* Add reason description */
    if( NULL != reason_ptr) {
        OSAL_snprintf(msg_ptr->msg.call.reasonDesc, ISI_EVENT_DESC_STRING_SZ, 
            "%s", reason_ptr);
    }
    else {
        msg_ptr->msg.call.reasonDesc[0] = '\0';
    }

    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_cancelCallModify() ========
 * This function is used to cancel a call modify offered by UA client.

 * callId : The identification number for the call to be modified.  This
 *          ID number was provided by ISI to the application when it
 *          initiated a call or delivered to the application through ISI
 *          event when a call was received..
 *
 * Returns Values:
 * ISI_RETURN_OK              : Call modified successfully.
 * ISI_RETURN_FAILED          : Call modified failed.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_cancelModifyCall(
    ISI_INP ISI_Id callId)
{
    ISID_CallId *call_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }

    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_CANCEL_MODIFY,
            ISIP_STATUS_INVALID, NULL, NULL)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();


    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_acceptCallModify() ========
 * This function is used to accept a call modify offered by remote party.
 *
 * callId : The identification number for the call to be modified.  This
 *          ID number was provided by ISI to the application when it
 *          initiated a call or delivered to the application through ISI
 *          event when a call was received..
 *
 * Returns Values:
 * ISI_RETURN_OK              : Call modified successfully.
 * ISI_RETURN_FAILED          : Call modified failed.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_acceptCallModify(
    ISI_INP ISI_Id callId)
{
    ISID_CallId *call_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }

    /* Set direction for ONHOLD state */
    if (ISI_CALL_STATE_ONHOLD == call_ptr->state) {
        if (ISI_SESSION_TYPE_AUDIO & call_ptr->type) {
            if (ISI_SESSION_DIR_RECV_ONLY == call_ptr->audioDir) {
                call_ptr->audioDir = ISI_SESSION_DIR_INACTIVE;
            }
            else if (ISI_SESSION_DIR_SEND_RECV == call_ptr->audioDir) {
                call_ptr->audioDir = ISI_SESSION_DIR_SEND_ONLY;
            }
        }
        if (ISI_SESSION_TYPE_VIDEO & call_ptr->type) {
            if (ISI_SESSION_DIR_RECV_ONLY == call_ptr->videoDir) {
                call_ptr->videoDir = ISI_SESSION_DIR_INACTIVE;
            }
            else if (ISI_SESSION_DIR_SEND_RECV == call_ptr->audioDir) {
                call_ptr->videoDir = ISI_SESSION_DIR_SEND_ONLY;
            }
        }
    }

    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_ACCEPT_MODIFY,
            ISIP_STATUS_INVALID, NULL, NULL)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_rejectCallModify() ========
 * This function is used to reject a call modify offered by remote party.
 *
 * callId : The identification number for the call to be modified.  This
 *          ID number was provided by ISI to the application when it
 *          initiated a call or delivered to the application through ISI
 *          event when a call was received..
 *
 * Returns Values:
 * ISI_RETURN_OK              : Call modified successfully.
 * ISI_RETURN_FAILED          : Call modified failed.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_rejectCallModify(
    ISI_INP ISI_Id  callId,
    ISI_INP char   *reason_ptr)
{
    ISID_CallId *call_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }

    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_REJECT_MODIFY,
            ISIP_STATUS_INVALID, NULL, reason_ptr)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_getCallState() ========
 * This function is used to get state of a call.
 *
 * callId : The identification number for the call in question.  This
 *          ID number was provided by ISI to the application when it
 *          initiated a call or delivered to the application through ISI
 *          event when a call was received.
 *
 * callState_ptr : This is a pointer to a memory location where ISI will write
 *                 the state of the call.  Possible call states are enumerated
 *                 in ISI_CallState.
 *
 * Return Values:
 * ISI_RETURN_OK              : Success.  The state will be in callState_ptr.
 * ISI_RETURN_FAILED          : Could not get the state.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_getCallState(
    ISI_Id         callId,
    ISI_CallState *callState_ptr)
{
    ISID_CallId *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!callState_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    *callState_ptr = call_ptr->state;
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_addCoderToCall() ========
 * This function is used to add or change a coder used for communication in an
 * active call. Once the coder is set via this function, the updated coder
 * settings can then be activated via ISI_modifyCall().  User can only add or
 * remove a coder from a call if the service associated with that call allows
 * the use of that coder.
 *
 * callId : The identification number for the call for which the coder is to
 *          be changed.  This ID number was provided by ISI to the application
 *          when it initiated a call or delivered to the application through a
 *          ISI event when a call was received.
 *
 * codeName_ptr : A null terminated string containing the name of coder.
 *
 * coderDescription_ptr : A null terminated string with coder description like
 *                        rate, priority, sdp-number etc.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Coder added/changed successfully.
 * ISI_RETURN_INVALID_CODER      : Coder not found.
 * ISI_RETURN_INVALID_CALL_ID    : Call ID is invalid.
 * ISI_RETURN_NOT_SUPPORTED      : Coder not supported by this service this
 *                                 call uses.
 * ISI_RETURN_NOT_INIT           : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Call linked list.
 */
ISI_Return ISI_addCoderToCall(
    ISI_Id  callId,
    char   *coderName_ptr,
    char   *coderDescription_ptr)
{
    ISID_CallId *call_ptr;
    ISIP_Coder  *cdr_ptr;
    ISIP_Coder  *e_ptr;
    ISIP_Coder   coder;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!coderName_ptr) {
        /* We must not have the coder name */
        return (ISI_RETURN_INVALID_CODER);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid */

        /* First see if the coder exits */
        cdr_ptr = ISID_coderFind(call_ptr->coders, ISI_CODER_NUM,
                coderName_ptr, coderDescription_ptr);
        if (cdr_ptr) {
            /* Found it so update it */
            if (coderDescription_ptr) {
                OSAL_strncpy(cdr_ptr->description, coderDescription_ptr,
                        sizeof(cdr_ptr->description));
            }
            ISID_unlockCalls();
            return (ISI_RETURN_OK);
        }

        /*
         * If we are here then it does not currently exist. Check coder against
         * supported types in the service
         */
        e_ptr = ISID_coderFind(call_ptr->service_ptr->coders,
                ISI_CODER_NUM, coderName_ptr, coderDescription_ptr);
        if (e_ptr) {
            /* populate */
            OSAL_strncpy(coder.szCoderName, coderName_ptr,
                    sizeof(coder.szCoderName));
            if (coderDescription_ptr) {
                OSAL_strncpy(coder.description, coderDescription_ptr,
                        sizeof(coder.description));
            }
            coder.relates = e_ptr->relates;
            if (ISID_coderAdd(call_ptr->coders, ISI_CODER_NUM,
                        &coder) == ISI_RETURN_OK) {
                ISID_unlockCalls();
                return (ISI_RETURN_OK);
            }
            else {
                /* If we are here we can't add anymore coders */
                ISID_unlockCalls();
                return (ISI_RETURN_INVALID_CODER);
            }
        }
        ISID_unlockCalls();
        return (ISI_RETURN_NOT_SUPPORTED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}

/*
 * ======== ISI_removeCoderFromCall() ========
 * This function is used to remove a coder used for communication from an active
 * call.  Once the coder is removed via this function, the updated coder
 * settings are then activated via ISI_modifyCall().  Users can only add or
 * remove a coder from a call if the service associated with that call
 * allows the use of that coder.
 *
 * callId : The identification number for the call for which the coder is to
 *          be removed.  This ID number was provided by ISI to the application
 *          when it initiated a call or delivered to the application through a
 *          ISI event when a call was received.
 *
 * codeName_ptr : A null terminated string containing the name of coder.
 *
 * Return Values:
 * ISI_RETURN_OK              : Coder removed successfully.
 * ISI_RETURN_INVALID_CODER   : Coder not found.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_removeCoderFromCall(
    ISI_Id  callId,
    char   *coderName_ptr)
{
    ISID_CallId *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!coderName_ptr) {
        /* We must have the coder name */
        return (ISI_RETURN_INVALID_CODER);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid.
         */
        if (ISID_coderRemove(call_ptr->coders, ISI_CODER_NUM,
                coderName_ptr) == ISI_RETURN_FAILED) {
            ISID_unlockCalls();
            /* Then the coder did not exist in the service */
            return (ISI_RETURN_INVALID_CODER);
        }
        ISID_unlockCalls();
        return (ISI_RETURN_OK);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}

/*
 * ======== ISI_setCallSessionDirection() ========
 * This function is used to change the session direction for an active call.
 * The session direction represents the direction of media flow (audio, video
 * etc.) in a particular call.  If session direction is set to
 * ISI_SESSION_DIR_RECV_ONLY, media flows in one direction from the remote agent
 * to the local agent.  If session direction is set to ISI_SESSION_DIR_SEND_ONLY
 * then media flows in one direction from local agent to remote agent.  If
 * session direction is set to ISI_SESSION_DIR_SEND_RECV, full duplex media
 * flows between the local and remote users.  If session direction is set to
 * ISI_SESSION_DIR_INACTIVE, then media stops flowing between them. Once the
 * session direction is set via this function then the direction can be
 * activated via a call to ISI_modifyCall().
 *
 * callId : The identification number for the call for which the session
 *          direction is to be changed.  This ID number was provided by ISI to
 *          the application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * callType : The session type of the call.  Possible call session type values
 *            are enumerated in ISI_SessionType.
 *
 * direction : The new session direction for the call.  Valid session
 *             directions are enumerated in ISI_SessionDirection.
 *
 * Return Values:
 * ISI_RETURN_OK                  : Session direction changed successfully.
 * ISI_RETURN_INVALID_SESSION_DIR : Invalid session direction specified.
 * ISI_RETURN_INVALID_CALL_ID     : Call ID is invalid.
 * ISI_RETURN_NOT_INIT            : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the linked list.
 */
ISI_Return ISI_setCallSessionDirection(
    ISI_Id               callId,
    ISI_SessionType      callType,
    ISI_SessionDirection direction)
{
    ISID_CallId *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid. */
        if (direction >= ISI_SESSION_DIR_INACTIVE &&
                direction < ISI_SESSION_DIR_INVALID) {
            if (ISI_SESSION_TYPE_AUDIO == callType) {
                call_ptr->audioDir = direction;
            }
            else if (ISI_SESSION_TYPE_VIDEO == callType) {
                call_ptr->videoDir = direction;
            }
            else {
                return (ISI_RETURN_INVALID_SESSION_DIR);
            }
            ISID_unlockCalls();
            return (ISI_RETURN_OK);
        }
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_SESSION_DIR);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}


/*
 * ======== ISI_getCallSessionDirection() ========
 * This function is used to get current direction of a call.
 *
 * callId : The identification number for the call for which the session
 *          direction is to be returned.  This ID number was provided by ISI
 *          to the application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * callType : The session type of the call.  Possible call session type values
 *            are enumerated in ISI_SessionType.
 *
 * callDir_ptr : A pointer to a memory location where ISI will write the
 *               session direction of the call.  Possible call direction values
 *               are enumerated in ISI_SessionDirection.
 *
 * Return Values:
 * ISI_RETURN_OK              : Success.
 * ISI_RETURN_FAILED          : Could not return the call direction.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_getCallSessionDirection(
    ISI_Id                callId,
    ISI_SessionType       callType,
    ISI_SessionDirection *callDir_ptr)
{
    ISID_CallId *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!callDir_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid.
         */
        if (ISI_SESSION_TYPE_AUDIO == callType) {
            *callDir_ptr = call_ptr->audioDir;
        }
        else if (ISI_SESSION_TYPE_VIDEO == callType) {
            *callDir_ptr = call_ptr->videoDir;
        }
        else {
            return (ISI_RETURN_INVALID_SESSION_DIR);
        }
        ISID_unlockCalls();
        return (ISI_RETURN_OK);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}

/*
 * ======== ISI_getCallHeader() ========
 * This function is used to get the header information of a call.  Call header
 * info is a combination of the call 'subject' and remote agent's 'address'
 * (Caller ID in POTS terminology).  Note that if there is no subject associated
 * with the call then subject_ptr will be an empty NULL terminated string.
 * ISI may write up to ISI_SUBJECT_STRING_SZ bytes to subject_ptr and up to
 * ISI_ADDRESS_STRING_SZ bytes in to the from_ptr.  Therefore applications
 * should provide buffers ISI_SUBJECT_STRING_SZ and ISI_ADDRESS_STRING_SZ bytes
 * long for both the subject_ptr and from_ptr respectively.
 *
 * callId : The identification number for the call for which the call header
 *          is to be returned.  This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * subject_ptr : A pointer to memory location where ISI will write the subject
 *               of the call.
 *
 * from _ptr : A pointer to memory location where ISI will write the address
 *             of remote agent.
 *
 * Return Values:
 * ISI_RETURN_OK              : Success.  Header returned.
 * ISI_RETURN_FAILED          : Header cannot be returned.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_getCallHeader(
    ISI_Id               callId,
    char                *subject_ptr,
    char                *from_ptr)
{
    ISID_CallId *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!subject_ptr || !from_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid.
         */
        if (call_ptr->szRemoteUri[0] == 0) {
            ISID_unlockCalls();
            return (ISI_RETURN_FAILED);
        }
        OSAL_strncpy(subject_ptr, call_ptr->szSubject, ISI_SUBJECT_STRING_SZ);
        OSAL_strncpy(from_ptr, call_ptr->szRemoteUri, ISI_ADDRESS_STRING_SZ);
        ISID_unlockCalls();
        return (ISI_RETURN_OK);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}


/*
 * ======== ISI_getSupsrvHeader() ========
 * This function is used to get the supsrv related header information of a call.
 * Supsrv header info is a combination of the bit masked flag 'supsrvHfExist'
 *
 * callId : The identification number for the call for which the supsrv header
 *          is to be returned.  This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * supsrvHfExist_ptr : A pointer to a bit masked flag to indicate whether
 *          certain suprv hf existed or not. HF values are passed
 *          by other parameters
 *
 * Return Values:
 * ISI_RETURN_OK              : Success.  Header returned.
 * ISI_RETURN_FAILED          : Header cannot be returned.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_getSupsrvHeader(
    ISI_Id   callId,
    int     *supsrvHfExist_ptr)
{
    ISID_CallId *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!supsrvHfExist_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid.
         */
        if (call_ptr->szRemoteUri[0] == 0) {
            ISID_unlockCalls();
            return (ISI_RETURN_FAILED);
        }
        *supsrvHfExist_ptr = call_ptr->supsrvHfExist;
        ISID_unlockCalls();
        return (ISI_RETURN_OK);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}

/*
 * ======== ISI_getSupsrvHistoryInfo() ========
 * This function is used to get the supsrv history information of a call.
 *
 * callId : The identification number for the call for which the supsrv header
 *          is to be returned.  This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * historyInfo_ptr : A pointer to memory location where ISI will write the
 *          history-info HF value. Noted that it could be empty even the
 *          supsrvHfExist flag indicated there is a such field.
 *          The reason could be privacy policy.
 *
 * Return Values:
 * ISI_RETURN_OK              : Success.  Header returned.
 * ISI_RETURN_FAILED          : Header cannot be returned.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_getSupsrvHistoryInfo(
    ISI_Id   callId,
    char    *historyInfo_ptr)
{
    ISID_CallId *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!historyInfo_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid.
         */
        if (call_ptr->szRemoteUri[0] == 0) {
            ISID_unlockCalls();
            return (ISI_RETURN_FAILED);
        }
        OSAL_strncpy(historyInfo_ptr, call_ptr->historyInfo,
                ISI_HISTORY_INFO_STRING_SZ);
        ISID_unlockCalls();
        return (ISI_RETURN_OK);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}

/*
 * ======== ISI_getCallSessionType() ========
 * This function is used to get session type of a call.
 *
 * callId : The identification number for the call for which the session type
 *          is to be returned.  This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * callType_ptr : A pointer to a memory location where ISI will write the
 *                session type of the call.  Possible call session type values
 *                are enumerated in ISI_SessionType.
 *
 * Return Values:
 * ISI_RETURN_OK              : Success.
 * ISI_RETURN_FAILED          : could not return the call session type
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_getCallSessionType(
    ISI_Id           callId,
    uint16          *callType_ptr)
{
    ISID_CallId *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!callType_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid.
         */
        *callType_ptr = call_ptr->type;
        ISID_unlockCalls();
        return (ISI_RETURN_OK);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}

/*
 * ======== ISI_updateCallSession() ========
 * This function is used to update the call session such as adding or removing video media,
 * change the direction of a particular media flow. Once the session is updated via this
 * function, changes to the call session can be activated via a call to ISI_modifyCall().
 *
 *
 * callId : The identification number for the call for which the session type
 *          is to be set.  This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * mediaAttribute_ptr : A null terminated string in xml format. This string
 *          represents the changes to be done at the media level to the call
 *          session. Please refer to _isi_xml.h for more details on various
 *          tags and attributes used.
 * e.g -
 * <media>
 *     <audio enabled="true"
 *         direction="sendrecv"
 *         secure="false"
 *         emergency="false"
 *         useRtpAVPF="false"
 *         maxBandwidth="0"/>
 *     <video enabled="true"
 *         direction="sendrecv"
 *         secure="false"
 *         emergency="false"
 *         useRtpAVPF="true"
 *         maxBandwidth="1024"/>
 * </media>
 *
 * Return Values:
 * ISI_RETURN_OK              : Success.
 * ISI_RETURN_FAILED          : Failed to parse the xml string.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_updateCallSession(
    ISI_Id  callId,
    char   *mediaAttribute_ptr)
{
    ISID_CallId *call_ptr;
    ISI_Return ret;

    OSAL_logMsg("ISI_updateCallSession %s \n",    mediaAttribute_ptr);
    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid.     */
        ret = ISI_decodeMediaAttributeXMLDoc(mediaAttribute_ptr,
                call_ptr);
        ISID_unlockCalls();
        return ret;
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}

/*
 * ======== ISI_getCoderDescription() ========
 * This function is used to get the description of a coder
 * used in the call.
 *
 * callId : The identification number of the call to get the coder priority
 *          information.  This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * codername_ptr : A NULL terminated string containing the name of the coder.
 *
 * coderDescription_ptr : A pointer to memory location where ISI will write
 *          coder description.
 *
 * Return Values:
 * ISI_RETURN_OK              : Coder  priority placed in coderprio_ptr.
 * ISI_RETURN_FAILED          : Could not return the coder priority
 * ISI_RETURN_INVALID_CODER   : Coder not found.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_getCoderDescription(
    ISI_Id  callId,
    char   *codername_ptr,
    char   *coderDescription_ptr)
{
    ISID_CallId *call_ptr;
    ISIP_Coder  *coder_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!codername_ptr) {
        return (ISI_RETURN_INVALID_CODER);
    }
    if (!coderDescription_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid.
         */
        coder_ptr = ISID_coderFind(call_ptr->coders, ISI_CODER_NUM,
                codername_ptr, coderDescription_ptr);
        if (coder_ptr) {
            /* Found it */
            OSAL_strncpy(coderDescription_ptr, coder_ptr->description,
                    ISI_CODER_DESCRIPTION_STRING_SZ);
            ISID_unlockCalls();
            return (ISI_RETURN_OK);
        }
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CODER);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}

/*
 * ======== ISI_acknowledgeCall() ========
 * This function is used to acknowledge an incoming call.  ISI uses underlying
 * protocol to inform calling party that the called party is being informed
 * of a new call.  Calling party can use this information to start local ring
 * back sequence.  Information about incoming call to be acknowledged was
 * delivered to application using a ISI event.
 *
 * callId : The identification number for the call for which to acknowledge.
 *          This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * Return Values:
 * ISI_RETURN_OK              : Call acknowledged.
 * ISI_RETURN_FAILED          : Call cannot be acknowledged, user may retry.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_acknowledgeCall(
    ISI_Id callId)
{
    ISID_CallId *call_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }

    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_ACK,
            ISIP_STATUS_INVALID, NULL, NULL)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_holdCall() ========
 * This function is used to place a call on hold.
 *
 * callId : The identification number of the call to place on hold.  This ID
 *          number was provided by ISI to the application when it initiated a
 *          call or delivered to the application through a ISI event when a call
 *          was received.
 *
 * Return Values:
 * ISI_RETURN_OK              : Call put on hold.
 * ISI_RETURN_FAILED          : Call cannot be put on hold, user may retry.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_holdCall(
    ISI_Id callId)
{
    ISID_CallId *call_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }

    /* Set the direction of the call following 3GPP TS 24.610 */
    if (ISI_SESSION_TYPE_AUDIO & call_ptr->type) {
        if (ISI_SESSION_DIR_RECV_ONLY == call_ptr->audioDir) {
            call_ptr->audioDir = ISI_SESSION_DIR_INACTIVE;
        }
        else {
            call_ptr->audioDir = ISI_SESSION_DIR_SEND_ONLY;
        }
    }
    if (ISI_SESSION_TYPE_VIDEO & call_ptr->type) {
        if (ISI_SESSION_DIR_RECV_ONLY == call_ptr->videoDir) {
            call_ptr->videoDir = ISI_SESSION_DIR_INACTIVE;
        }
        else {
            call_ptr->videoDir = ISI_SESSION_DIR_SEND_ONLY;
        }
    }

    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_HOLD,
            ISIP_STATUS_INVALID, NULL, NULL)) == NULL) {
        /* Reinstate the previous direction setting */
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        /* Reinstate the previous direction setting */
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_resumeCall() ========
 * This function is used to resume a call that was put on hold. The call must
 * have been on hold for the function to succeed. A call can only be resumed by
 * agent that put the call on hold.
 *
 * callId : The identification number of the call to resume.
 *          This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * Return Values:
 * ISI_RETURN_OK              : Call taken off hold.
 * ISI_RETURN_FAILED          : Call cannot be taken off hold, user may retry.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_resumeCall(
    ISI_Id callId)
{
    ISID_CallId  *call_ptr;
    ISIP_Message  *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }

    /* Set the call direction to be active again following 3GPP TS 24.610 */
    if (ISI_SESSION_TYPE_AUDIO & call_ptr->type) {
        if (ISI_SESSION_DIR_INACTIVE == call_ptr->audioDir) {
            call_ptr->audioDir = ISI_SESSION_DIR_RECV_ONLY;
        }
        else {
            call_ptr->audioDir = ISI_SESSION_DIR_SEND_RECV;
        }
    }
    if (ISI_SESSION_TYPE_VIDEO & call_ptr->type) {
        if (ISI_SESSION_DIR_INACTIVE == call_ptr->videoDir) {
            call_ptr->videoDir = ISI_SESSION_DIR_RECV_ONLY;
        }
        else {
            call_ptr->videoDir = ISI_SESSION_DIR_SEND_RECV;
        }
    }

    if (ISI_SESSION_TYPE_VIDEO & call_ptr->type) {
        /* If currently SEND_ONLY change to SEND_RECV. */
        if (call_ptr->videoDir == ISI_SESSION_DIR_SEND_ONLY) {
            call_ptr->videoDir = ISI_SESSION_DIR_SEND_RECV;
        }
        /* If currently INACTIVE change to RECV_ONLY. */
        if (call_ptr->videoDir == ISI_SESSION_DIR_INACTIVE) {
            call_ptr->videoDir = ISI_SESSION_DIR_RECV_ONLY;
        }
    }

    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_RESUME,
            ISIP_STATUS_INVALID, NULL, NULL)) == NULL) {
        /* Reinstate the previous direction setting */
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        /* Reinstate the previous direction setting */
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_acceptCall() ========
 * This function is used to accept an incoming call.  Information about
 * incoming call to be accepted was delivered to application using a ISI event.
 * Media will start flowing when call is accepted.
 *
 * callId : The identification number of the call to accept.
 *          This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * Return Values:
 * ISI_RETURN_OK              : Call accepted.
 * ISI_RETURN_FAILED          : Call cannot be accepted.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_acceptCall(
    ISI_Id callId)
{
    ISID_CallId *call_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    if ((msg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_ACCEPT,
            ISIP_STATUS_INVALID, NULL, NULL)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}




/*
 * ======== ISI_rejectCall() ========
 * This function is used to reject an incoming call. After a call is rejected,
 * all resources related to that call are freed and the callId becomes invalid.
 * Any further references to the call will return ISI_RETURN_INVALID_CALL_ID.
 * When application rejects a call, ISI uses underlying protocol to send busy to
 * the calling party.  It is responsibility of service provider to redirect
 * calling party to voice mail if required.
 *
 * callId : The identification number of the call to reject.
 *          This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * reason_ptr : The reason description for call rejection.
 *
 * Return Values:
 * ISI_RETURN_OK              : Call rejected.
 * ISI_RETURN_FAILED          : Call cannot be rejected, user may retry.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_rejectCall(
    ISI_Id  callId,
    char   *reason_ptr)
{
    ISID_CallId *call_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_REJECT,
            ISIP_STATUS_INVALID, NULL, NULL)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    /* Add reason description */
    if( NULL != reason_ptr) {
        OSAL_snprintf(msg_ptr->msg.call.reasonDesc, ISI_EVENT_DESC_STRING_SZ, 
            "%s", reason_ptr);
    }
    else {
        msg_ptr->msg.call.reasonDesc[0] = '\0';
    }
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_forwardCall() ========
 * This function is used to forward an incoming call.  After a call is forwarded
 * all resources related to that call are freed and the callId becomes invalid.
 * Any further references to the call will return ISI_RETURN_INVALID_CALL_ID.
 *
 * callId : The identification number of the call to forward.
 *          This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * to_ptr : A pointer to memory location containing a null terminated string
 *          representing the address to forward the call to.
 *
 * Return Values:
 * ISI_RETURN_OK              : Call forwarded.
 * ISI_RETURN_FAILED          : Call cannot be forwarded.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_forwardCall(
    ISI_Id  callId,
    char   *to_ptr)
{
    ISID_CallId *call_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!to_ptr || *to_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_FORWARD,
            ISIP_STATUS_INVALID, to_ptr, NULL)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);

}

/*
 * ======== ISI_blindTransferCall() ========
 * This function is used to blind transfer a call.  A Call to this function
 * starts a transfer sequence.  The status of the transfer is delivered to
 * the application via ISI events.
 *
 * A successful blind transfer proceeds as follows:
 *  1.Transferee is referred to transfer location.
 *  2.Transferor terminates call with transferee.
 *
 * After a call transfer is completed, the application will receive a
 * ISI_EVENT_CALL_XFER_COMPLETED event.  At this time all resources related to
 * the transferred call are freed and the callId becomes invalid.  Any further
 * references to the call will return ISI_RETURN_INVALID_CALL_ID.
 * If a call transfer fails, the application is notified via a
 * ISI_EVENT_CALL_XFER_FAILED event but the call is completely terminated.
 *
 * callId : The identification number of the call to be transferred.
 *          This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * to_ptr : A pointer to a memory location containing a null terminated string
 *          representing the address of the transfer target.
 *
 * Return Values:
 * ISI_RETURN_OK              : Call transfer started.
 * ISI_RETURN_FAILED          : Call cannot be transferred.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_blindTransferCall(
    ISI_Id  callId,
    char   *to_ptr)
{
    ISID_CallId  *call_ptr;
    ISIP_Message  *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Validate the to_ptr */
    if (!to_ptr || *to_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_TRANSFER_BLIND,
            ISIP_STATUS_INVALID, to_ptr, call_ptr->szSubject)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    /* Immediately disconnect now */
    if ((msg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_TERMINATE,
            ISIP_STATUS_INVALID, NULL, NULL)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}


/*
 * ======== ISI_attendedTransferCall() ========
 * This function is used to perform an attended call transfer. A Call to this
 * function starts a transfer sequence.  The status of transfer sequence is
 * delivered to the application via ISI events.
 * A successful attended call transfer proceeds as follows:
 *  1.Transferee is put on hold (application's responsibility)
 *  2.Transferee is referred to transfer location (application's responsibility)
 *  3.Transferee tells transferor of successful transfer.
 *  4.Transferor terminates call with transferee.
 *
 * After a call transfer is completed, the application will receive a
 * ISI_EVENT_CALL_XFER_COMPLETED event. The call is then considered to be
 * disconnected, all resources related to the transferred call are freed and the
 * callId becomes invalid. Any further references to the call will return
 * ISI_RETURN_INVALID_CALL_ID.  If a call transfer fails, the application is
 * informed of the failure via the ISI_EVENT_CALL_XFER_FAILED event and the
 * original call is restored.
 *
 * Unlike blind transfer, the call can be restored/recovered if the transfer
 * fails.
 *
 * A failed attended transfer example has following sequence:
 *  1.Application puts call to be transferred on hold.
 *  2.Application attended transfers calls.
 *  3.Transferee cannot contact transfer location hence transfer fails.
 *  4.Transferee notifies transferor that call transfer failed (application
 *    receives ISI event ISI_EVENT_CALL_XFER_FAILED).
 *  5.Application takes the call off hold and conversation continues.
 *
 * A passed attended transfer example has following sequence:
 *  1.Application puts call to be transferred on hold.
 *  2.Application attended transfers calls.
 *  3.Transferee successfully contacts the transfer location.
 *  4.Transferee notifies transferor that call transfer succeeded (application
 *    receives ISI event ISI_EVENT_CALL_XFER_COMPLETED).
 *  5.Application disconnects call with transferee.
 *
 *
 * callId : The identification number of the call to be transferred.
 *          This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * to_ptr : A pointer to a memory location containing a null terminated string
 *          representing the address of the transfer target.
 *
 * Return Values:
 * ISI_RETURN_OK              : Call transfer started.
 * ISI_RETURN_FAILED          : Call cannot be transferred.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_attendedTransferCall(
    ISI_Id  callId,
    char   *to_ptr)
{
    ISID_CallId  *call_ptr;
    ISIP_Message  *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Validate the to_ptr parameter */
    if (!to_ptr || *to_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr,
            ISIP_CALL_REASON_TRANSFER_ATTENDED, ISIP_STATUS_INVALID,
            to_ptr, NULL)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_consultativeTransferCall() ========
 * This function is used to perform a call transfer after consultation. A Call to this
 * function starts a transfer sequence between 2 existing calls.  The status of the
 * transfer sequence is delivered to the application via ISI events.
 * A successful call transfer proceeds as follows:
 *  1.Transferee is put on hold (application's responsibility).
 *  2.A second call is established with the transfer target (application's responsibility).
 *  3.Transferee is referred to transfer target (application's responsibility).
 *  4.Transferee tells transferor of successful transfer.
 *  5.Transferor terminates call with transferee.
 *
 * After a call transfer is completed, the application will receive a
 * ISI_EVENT_CALL_XFER_COMPLETED event. The call should be considered
 * disconnected and the application should disconnect the call legs
 * if they haven't already been disconnected by the remote parties.
 *
 * A failed attended transfer example has following sequence:
 *  1.Application puts call to be transferred on hold.
 *  2.A second call is established with the transfer target.
 *  3.Application executes the transfer.
 *  4.transfer fails.
 *  5.Transferee notifies transferor that call transfer failed (application
 *    receives ISI event ISI_EVENT_CALL_XFER_FAILED).
 *  5.Application takes the call off hold and conversation continues.
 *
 * A passed attended transfer example has following sequence:
 *  1.Application puts call to be transferred on hold.
 *  2.A second call is established with the transfer target.
 *  3.Application exectures the transfer.
 *  3.Transferee successfully contacts the transfer location.
 *  4.Transferee notifies transferor that call transfer succeeded (application
 *    receives ISI event ISI_EVENT_CALL_XFER_COMPLETED).
 *  5.Application disconnects call with transferee.
 *
 *
 * callId : The identification number of the call to be transferred.
 *          This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * toCallId : The identification number of the transfer targeted call.
 *          This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.

 *
 * Return Values:
 * ISI_RETURN_OK              : Call transfer started.
 * ISI_RETURN_FAILED          : Call cannot be transferred.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_consultativeTransferCall(
    ISI_Id  callId,
    ISI_Id  toCallId)
{
    ISID_CallId  *call_ptr;
    ISIP_Message *msg_ptr;
    char         *transferTo_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    /* See if the transfer target is active. */
    if (ISID_callGet(toCallId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    /* let's get a reference to the address of the transfer target. */
    transferTo_ptr = call_ptr->szRemoteUri;
    /* Let's the call we are actually interested in. */
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr,
            ISIP_CALL_REASON_TRANSFER_CONSULT, ISIP_STATUS_INVALID,
            transferTo_ptr, NULL)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    /* Let's also add the callId of the transfer target. */
    msg_ptr->msg.call.transferTargetCallId = toCallId;
    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_confTransferCall() ========
 * This function is used to perform an conf call transfer. A Call to this
 * function starts a transfer sequence.  The status of transfer sequence is
 * delivered to the application via ISI events.
 * A successful conf call transfer proceeds as follows:
 *  * Transferee is referred to transfer location 
 *  * Transferee tells transferor of successful transfer.
 *
 * After a conf call transfer is completed, the application will receive a
 * ISI_EVENT_CALL_XFER_COMPLETED event. The action is then considered to be
 * completed. The conferefece focus have joined the new participant to the conf all.
 *
 * The call can be restored/recovered if the transfer
 * fails.
 *
 * A failed attended transfer example has following sequence:
 *  *.Application attended transfers calls.
 *  *.Transferee notifies transferor that call transfer failed (application
 *    receives ISI event ISI_EVENT_CALL_XFER_FAILED).
 *  *.Application continue the conf call without considering the user added
 *
 * A passed attended transfer example has following sequence:
 *  2.Application attended transfers calls.
 *  3.Transferee successfully contacts the transfer location.
 *  4.Transferee notifies transferor that call transfer succeeded (application
 *    receives ISI event ISI_EVENT_CALL_XFER_COMPLETED). Contact field
 *    will contain the conf-call call uri to be used in later dialog identification
 *  5.Application update the conf-call member via subscribed event
 *
 * callId : The identification number of the call to be transferred.
 *          This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *          Usually, this is the conf-call callId. In ad-hoc case,
 *          this is the existing call to be converted to conf-call
 *
 * to_ptr : A pointer to a memory location containing a null terminated string
 *          representing the address of the transfer target.
 *          Usually, this is the new participant to be joined. In ad-hoc case,
 *          this is the conf-call factory.
 * 
 * rsrcList_ptr :For single participant, this is NULL.
 *          in case of multiple participants to be joined at once, this is
 *          the list of users.
 *
 * Return Values:
 * ISI_RETURN_OK              : Call transfer started.
 * ISI_RETURN_FAILED          : Call cannot be transferred.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */

ISI_Return ISI_confTransferCall(
    ISI_Id  callId,
    char   *to_ptr,
    char   *rsrcList_ptr)
{
    ISID_CallId  *call_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }

    /* Now prepare the protocol message */
    if ((msg_ptr = ISIM_updateCall(call_ptr,
            ISIP_CALL_REASON_TRANSFER_CONF, ISIP_STATUS_INVALID,
            to_ptr, NULL)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }

    if (NULL != rsrcList_ptr) {
        /*  copy particiants list */
        OSAL_strncpy(msg_ptr->msg.call.participants, rsrcList_ptr, ISI_LONG_ADDRESS_STRING_SZ);
    } else {
        msg_ptr->msg.call.participants[0] = '\0';
    }

    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}


/*
 * ======== ISI_initiateConfCall() ========
 * This function is used to initiate a call to a remote agent using a service
 * specified by serviceId.  ISI will generate the status of initiated call to
 * application using ISI events. Calls inherit default call properties from
 * the service used to initiate calls.  For example, if certain coders and
 * Caller ID blocking was enabled then those service settings are taken into
 * account when placing this call.
 *
 * callId_ptr : A pointer to a unique identification number for the call
 *              that is initiated.  This unique ID is returned by ISI layer
 *              to user application for future references involving the call.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * to_ptr : A null terminated string containing the address of the remote agent
 *          to call.
 *
 * subject_ptr : A null terminated string containing 'subject' of call.
 *               If subject_ptr is set to null, no subject will be used.
 *
 * cidType :  Describes the Caller Id policy, enumerated in ISI_SessionCidType.
 *
 * mediaAttribute_ptr : A null terminated string in xml format. This string
 *          represents the changes to be done at the media level to the call
 *          session. Please refer to _isi_xml.h for more details on various
 *          tags and attributes used.
 * e.g -
 * <media>
 *     <audio enabled="true"
 *         direction="sendrecv"
 *         secure="false"
 *         useRtpAVPF="false"
 *         maxBandwidth="0"/>
 *     <video enabled="true"
 *         direction="sendrecv"
 *         secure="false"
 *         useRtpAVPF="true"
 *         maxBandwidth="0"/>
 * </media>
 *
 * Return Values:
 * ISI_RETURN_OK                   : Call initiation successful.
 * ISI_RETURN_FAILED               : Call initiation failed.
 * ISI_RETURN_INVALID_SERVICE_ID   : Service ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the Call linked list.
 */
ISI_Return ISI_initiateConfCall(
    ISI_Id               *callId_ptr,
    ISI_Id                serviceId,
    char                 *to_ptr,
    char                 *subject_ptr,
    ISI_INP ISI_SessionCidType cidType,
    ISI_INP char         *mediaAttribute_ptr,
    char                 *rsrcList_ptr)
{
    ISID_CallId     *call_ptr;
    ISID_ServiceId  *service_ptr;
    ISIP_Message     *msg_ptr;
    ISI_Id            callId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!callId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Validate the to_ptr */
    if (!to_ptr || *to_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        printf("##lock error\n");
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid, we don't care if the service is
         * activated.  This is needed for VCC.
         */
        if ((call_ptr = ISID_callCreateFromXml(service_ptr, to_ptr, subject_ptr,
                cidType,mediaAttribute_ptr)) == NULL) {
            ISID_unlockCalls();
            return (ISI_RETURN_FAILED);
        }
        else {
            /* Add it to the database.  Init the callId pointer to zero
             * This indicates to the DB that we wish for the DB to determine
             * the CallId.
             */
            callId = 0;
            if (ISID_callAdd(call_ptr, &callId) == ISI_RETURN_FAILED) {
                /* Free the memory and return */
                ISI_free(call_ptr, ISI_OBJECT_CALL_ID);
                ISID_unlockCalls();
                return (ISI_RETURN_FAILED);
            }
            /* Now notify the protocol */
            if (ISIM_initiateCall(service_ptr, call_ptr,
                    &msg_ptr) == ISI_RETURN_INVALID_ADDRESS) {
                /* Then there was a problem with the to_ptr field */
                ISID_callDestroy(callId);
                ISID_unlockCalls();
                return (ISI_RETURN_FAILED);
            }
            
            /*  hack reason */
            msg_ptr->msg.call.reason = ISIP_CALL_REASON_INITIATE_CONF;
            
            /*  copy particiants list */
            OSAL_strncpy(msg_ptr->msg.call.participants, rsrcList_ptr, 
                    ISI_LONG_ADDRESS_STRING_SZ);
            if (ISIC_appMsg(call_ptr, msg_ptr) == ISI_RETURN_FAILED) {
                ISID_callDestroy(callId);
                ISID_unlockCalls();
                return (ISI_RETURN_FAILED);
            }
            *callId_ptr = callId;
            ISID_unlockCalls();
            return (ISI_RETURN_OK);
        }
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}


/*
 * ======== ISI_conferenceKickCall() ========
 *
 * This function is used to perform removing a call from PS conference by
 * sending a REFER with BYE. 
 *
 * callId : The identification number of the conference call.
 *
 * participant_ptr : NULL terminated string of participant's URI.
 *
 * Return Values:
 * ISI_RETURN_OK              : Call transfer started.
 * ISI_RETURN_FAILED          : Call cannot be transferred.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_conferenceKickCall(
    ISI_Id  callId,
    char   *participant_ptr)
{
    ISID_CallId  *call_ptr;
    ISIP_Message *msg_ptr;

    if ((NULL == participant_ptr) || (0 == participant_ptr[0])) {
        return (ISI_RETURN_FAILED);
    }

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    /* See if the transfer target is active. */
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }

    /* Let's the call we are actually interested in. */
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    /* Now notify the protocol */
    if ((msg_ptr = ISIM_updateCall(call_ptr,
            ISIP_CALL_REASON_CONF_KICK, ISIP_STATUS_INVALID,
            participant_ptr, NULL)) == NULL) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }

    if (ISIC_appMsg(call_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_generateTone() ========
 * This function is used to generate a tone locally on a specific call.
 * Applications typically use this function to generate call progress tones or
 * to generate DTMF tones to the local user's speaker.  Tones can be generated
 * for a specific time or forever pending the value of the duration parameter.
 *
 * callId = The identification number for the call.  This value was returned
 *          from ISI_initiateCall() or from the ISI_EVENT_CALL_INCOMING event.
 *
 * tone = The tone type to be played.  Tone types are enumerated in
 *        ISI_AudioTone. Possible values are: ISI_TONE_DTMF_0 through
 *        ISI_TONE_DTMF_9, ISI_TONE_DTMF_A through ISI_TONE_DTMF_D,
 *        ISI_TONE_DTMF_STAR, ISI_TONE_DTMF_POUND,  ISI_TONE_RINGBACK,
 *        ISI_TONE_CALLWAITING.
 *
 * duration = The duration in milliseconds to generate the tone.
 *
 * Return Values:
 * ISI_RETURN_OK              : The tone command was successfully issued.
 * ISI_RETURN_FAILED          : The tone command failed.
 * ISI_RETURN_INVALID_TONE    : The tone parameter was invalid.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_generateTone(
    ISI_Id        callId,
    ISI_AudioTone tone,
    int           duration)
{
    ISIP_Message *msg_ptr;
    ISID_CallId  *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Verify the tone is valid. */
    if ((tone < ISI_TONE_DTMF_0) || (tone >= ISI_TONE_LAST)) {
        return (ISI_RETURN_INVALID_TONE);
    }

    /* Lock the call list */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }

    /* Populate an ISIP Message and notify the protocol */
    if (NULL == (msg_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTART_CMD,
            tone, duration))) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISIM_appMsg(msg_ptr);
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_stopTone() ========
 * This function is used to stop a tone that is being generated locally on a
 * specific call.  Applications typically use this function to stop call
 * progress tones to the local user's speaker.
 *
 * callId = The identification number for the call.  This value was returned
 *          from ISI_initiateCall() or from the ISI_EVENT_CALL_INCOMING event.
 *
 * Return Values:
 * ISI_RETURN_OK              : The stop tone command was successfully issued.
 * ISI_RETURN_FAILED          : The tone command failed.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Call linked list.
 */
ISI_Return ISI_stopTone(
    ISI_Id        callId)
{
    ISIP_Message *msg_ptr;
    ISID_CallId  *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the call list */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        ISID_unlockCalls();
        return (ISI_RETURN_INVALID_CALL_ID);
    }

    /* Populate an ISIP Message and notify the protocol */
    if (NULL == (msg_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTOP_CMD,
            ISI_TONE_NONE, 0))) {
        ISID_unlockCalls();
        return (ISI_RETURN_FAILED);
    }
    ISIM_appMsg(msg_ptr);
    ISID_unlockCalls();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_sendTelEventToRemote() ========
 * This function is used to send a telephone event to the remote agent in a
 * call. If the telephone event is delivered to remote agent, ISI notifies
 * application of successful delivery via a ISI_EVENT_TEL_EVENT_SEND_OK event.
 * If the telephone event delivery fails, ISI notifies application of failure
 * via ISI_EVENT_TEL_EVENT_SEND_FAILED event.
 *
 * evtId_ptr : A pointer to memory location where ISI will write a unique
 *             telephone event identifier representing this telephone event
 *             transaction. ISI includes this same identifier in the
 *             ISI_EVENT_TEL_EVENT_SEND_OK or
 *             ISI_EVENT_TEL_EVENT_SEND_FAILED events when notify the
 *             application of the success or failure of the transaction.
 *
 * callId : The identification number of the call to send the telephone event
 *          within. This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * telEvent : The telephone event type to be sent to the remote agent.
 *            Telephone event types are enumerated in ISI_TelEvent.
 *
 * arg0 : Contains additional info about the event to be send.
 *        See API Doc for more details.
 *
 * arg1 : Contains additional info about the event to be send.
 *        See API Doc for more details.
 *
 * Return Values:
 * ISI_RETURN_OK                : The telephone event transaction started
 *                                successfully.
 * ISI_RETURN_FAILED            : The telephone event transaction failed.
 * ISI_RETURN_INVALID_TEL_EVENT : Invalid telEvent parameter.
 * ISI_RETURN_INVALID_CALL_ID   : Call ID is invalid.
 * ISI_RETURN_NOT_INIT          : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR       : Could not lock the Tel Event linked list.
 */
ISI_Return ISI_sendTelEventToRemote(
    ISI_Id       *evtId_ptr,
    ISI_Id        callId,
    ISI_TelEvent  telEvent,
    int           arg0,
    int           arg1)
{
    ISID_CallId     *call_ptr;
    ISID_EvtId      *evt_ptr;
    ISI_Id           evtId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!evtId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Verify the tel event is valid.  Currently only DTMF is supported */
    if (telEvent != ISI_TEL_EVENT_DTMF && telEvent != ISI_TEL_EVENT_DTMF_OOB &&
            telEvent != ISI_TEL_EVENT_FLASHHOOK) {
        return (ISI_RETURN_INVALID_TEL_EVENT);
    }

    /* Lock the events list */
    if (ISID_lockEvts() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        ISID_unlockEvts();
        return (ISI_RETURN_INVALID_CALL_ID);
    }

    if ((evt_ptr = ISID_evtCreate()) == NULL) {
        ISID_unlockEvts();
        return (ISI_RETURN_FAILED);
    }
    evt_ptr->call_ptr = call_ptr;
    evt_ptr->service_ptr = call_ptr->service_ptr;

    /*
     * Before you add this object, make sure that evtId is zero.
     * This will force the DB sub module to assign an id
     */
    evtId = 0;
    if (ISID_evtAdd(evt_ptr, &evtId) == ISI_RETURN_FAILED) {
        /* Free the memory and return */
        ISI_free(evt_ptr, ISI_OBJECT_EVT_ID);
        ISID_unlockEvts();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISIP Message and notify the protocol */
    ISIM_telEvt(evt_ptr, telEvent, call_ptr->szRemoteUri, arg0, arg1);
    ISIE_appMsg(evt_ptr, &evt_ptr->isiMsg);
    *evtId_ptr = evtId;
    ISID_unlockEvts();
    return (ISI_RETURN_OK);
}


/*
 * ======== ISI_sendTelEventStringToRemote() ========
 * This function is used to send a telephone event to the remote agent in a
 * call. If the telephone event is delivered to remote agent, ISI notifies
 * application of successful delivery via a ISI_EVENT_TEL_EVENT_SEND_OK event.
 * If the telephone event delivery fails, ISI notifies application of failure
 * via ISI_EVENT_TEL_EVENT_SEND_FAILED event.
 *
 * evtId_ptr : A pointer to memory location where ISI will write a unique
 *             telephone event identifier representing this telephone event
 *             transaction. ISI includes this same identifier in the
 *             ISI_EVENT_TEL_EVENT_SEND_OK or
 *             ISI_EVENT_TEL_EVENT_SEND_FAILED events when notify the
 *             application of the success or failure of the transaction.
 *
 * callId : The identification number of the call to send the telephone event
 *          within. This ID number was provided by ISI to the
 *          application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * telEvent : The telephone event type to be sent to the remote agent.
 *            Telephone event types are enumerated in ISI_TelEvent.
 *
 * string_ptr : Contains string of digits (
 *        See API Doc for more details.

 *
 * Return Values:
 * ISI_RETURN_OK                : The telephone event transaction started
 *                                successfully.
 * ISI_RETURN_FAILED            : The telephone event transaction failed.
 * ISI_RETURN_INVALID_TEL_EVENT : Invalid telEvent parameter.
 * ISI_RETURN_INVALID_CALL_ID   : Call ID is invalid.
 * ISI_RETURN_NOT_INIT          : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR       : Could not lock the Tel Event linked list.
 */
ISI_Return ISI_sendTelEventStringToRemote(
    ISI_OUT ISI_Id       *evtId_ptr,
    ISI_INP ISI_Id        callId,
    ISI_INP ISI_TelEvent  telEvent,
    ISI_INP char         *string_ptr,
    ISI_INP int           durationMs)
{
    
    ISID_CallId     *call_ptr;
    ISID_EvtId      *evt_ptr;
    ISI_Id           evtId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!evtId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Verify the tel event is valid.  Currently only DTMF is supported */
    if (telEvent != ISI_TEL_EVENT_DTMF_STRING) {
        return (ISI_RETURN_INVALID_TEL_EVENT);
    }

    /* Lock the events list */
    if (ISID_lockEvts() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        ISID_unlockEvts();
        return (ISI_RETURN_INVALID_CALL_ID);
    }

    if ((evt_ptr = ISID_evtCreate()) == NULL) {
        ISID_unlockEvts();
        return (ISI_RETURN_FAILED);
    }
    evt_ptr->call_ptr = call_ptr;
    evt_ptr->service_ptr = call_ptr->service_ptr;

    /*
     * Before you add this object, make sure that evtId is zero.
     * This will force the DB sub module to assign an id
     */
    evtId = 0;
    if (ISID_evtAdd(evt_ptr, &evtId) == ISI_RETURN_FAILED) {
        /* Free the memory and return */
        ISI_free(evt_ptr, ISI_OBJECT_EVT_ID);
        ISID_unlockEvts();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISIP Message and notify the protocol */
    ISIM_telEvtString(evt_ptr, telEvent, call_ptr->szRemoteUri, string_ptr, 
            durationMs);

    ISIE_appMsg(evt_ptr, &evt_ptr->isiMsg);
    *evtId_ptr = evtId;
    ISID_unlockEvts();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_getTelEventFromRemote() ========
 * This function is used to get a telephone event from the remote agent.
 * Applications call this function to get the telephone event received.
 * ISI informs applications about received telephone events via the
 * ISI_EVENT_TEL_EVENT_RECEIVED event.
 *
 * evtId :   The ID number of the event that the application wants to retrieve.
 *           This is the 'id' value that was provided to the application when it
 *           received a ISI_EVENT_TEL_EVENT_RECEIVED event.
 *
 * callId_ptr : A pointer to a ISI_Id.  ISI will write the callId that the event
 *              belongs to if the function is successful.
 *
 * telEvent_ptr : A pointer to a ISI_TelEvt object, ISI will write the
 *               enumerated tel event 'type' that was received.
 *
 * arg0_ptr : A pointer to a memory location that will be populated with the
 *            first argument of the tel event.
 *
 * arg1_ptr : A pointer to a memory location that will be populated with the
 *            second argument of the tel event.
 *
 * from_ptr : A pointer to a memory location that will be populated with the
 *            remote party that sent the tel event.
 *
 * dateTime_ptr : A pointer to a memory location that will be populated with the
 *            date and time of the tel event if the remote party reported a
 *            date and time.  The format of this string is dependant on the
 *            underlying protocol and or server.
 *
 * Return Values:
 * ISI_RETURN_OK                   : The telephone event successfully retrieved.
 * ISI_RETURN_FAILED               : The telephone event type could not be
 *                                   written to telEvent_ptr.
 * ISI_RETURN_INVALID_TEL_EVENT_ID : Invalid telephone event ID.
 * ISI_RETURN_NOT_INIT             : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the Tel Event linked list.
 */
ISI_Return ISI_getTelEventFromRemote(
    ISI_Id        evtId,
    ISI_Id       *callId_ptr,
    ISI_TelEvent *telEvent_ptr,
    int          *arg0_ptr,
    int          *arg1_ptr,
    char         *from_ptr,
    char         *dateTime_ptr)
{
    ISID_EvtId    *evt_ptr;
    ISIP_TelEvent *t_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!callId_ptr || !telEvent_ptr || !arg0_ptr || !arg1_ptr || !from_ptr ||
            !dateTime_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of texts */
    if (ISID_lockEvts() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the tel event in the database via the evtId */
    if (ISID_evtGet(evtId, &evt_ptr) != ISI_RETURN_OK) {
        /* It didn't exist */
        ISID_unlockEvts();
        return (ISI_RETURN_INVALID_TEL_EVENT_ID);
    }
    if (evt_ptr->call_ptr) {
        *callId_ptr = evt_ptr->call_ptr->e.desc;
        OSAL_strncpy(from_ptr, evt_ptr->call_ptr->szRemoteUri,
                ISI_ADDRESS_STRING_SZ);
    }
    else {
        *callId_ptr = 0;
        OSAL_strncpy(from_ptr, evt_ptr->isiMsg.msg.event.from,
                ISI_ADDRESS_STRING_SZ);
    }
    t_ptr = &evt_ptr->isiMsg.msg.event;
    if (t_ptr->evt == ISI_TEL_EVENT_INVALID) {
        /* It's not valid */
        ISID_unlockEvts();
        return (ISI_RETURN_FAILED);
    }
    *telEvent_ptr = t_ptr->evt;
    if (ISI_TEL_EVENT_DTMF_DETECT == *telEvent_ptr) {
        *arg0_ptr = t_ptr->settings.dtmfDetect.digit;
        *arg1_ptr = t_ptr->settings.dtmfDetect.edgeType;
    }
    else {
        *arg0_ptr = t_ptr->settings.args.arg0;
        *arg1_ptr = t_ptr->settings.args.arg1;
    }
    OSAL_strncpy(dateTime_ptr, evt_ptr->isiMsg.msg.event.dateTime,
                ISI_DATE_TIME_STRING_SZ);
    ISID_evtDestroy(evtId);
    ISID_unlockEvts();
    return (ISI_RETURN_OK);
}

/*
 * ======== DEPRECATED ========
 * ======== ISI_getTelEventResponse() ========
 * DEPRECATED: Do not use this routine.  There should be no reason
 * to use this routine.  When the response is received a
 * ISI_EVENT_TEL_EVENT_SEND_OK or ISI_EVENT_TEL_EVENT_SEND_FAILED event
 * is generated with the ISI id of the event.  There's no reason for
 * ISI to track the 'event type' (a.k.a. ISI_TelEvent).  That's the
 * applications responsibility.
 *
 * This function is used to get a telephone event from the remote agent.
 * Applications call this function to get the telephone event received.
 * ISI informs applications about received telephone events via the
 * ISI_EVENT_TEL_EVENT_RECEIVED event.
 *
 * evtId :   The ID number of the event that the application wants to retrieve.
 *           This is the 'id' value that was provided to the application when it
 *           received a ISI_EVENT_TEL_EVENT_RECEIVED event.
 *
 * telEvent_ptr : A pointer to a ISI_TelEvt object, ISI will write the
 *               enumerated tel event 'type' that was received.
 *
 *
 * Return Values:
 * ISI_RETURN_OK                   : The telephone event successfully retrieved.
 * ISI_RETURN_FAILED               : The telephone event type could not be
 *                                   written to telEvent_ptr.
 * ISI_RETURN_INVALID_TEL_EVENT_ID : Invalid telephone event ID.
 * ISI_RETURN_NOT_INIT             : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the Tel Event linked list.
 */

ISI_Return ISI_getTelEventResponse(
    ISI_Id        evtId,
    ISI_TelEvent *telEvent_ptr)
{
    ISID_EvtId    *evt_ptr;
    ISIP_TelEvent *t_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!telEvent_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of texts */
    if (ISID_lockEvts() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the tel event in the database via the evtId */
    if (ISID_evtGet(evtId, &evt_ptr) != ISI_RETURN_OK) {
        /* It didn't exist */
        ISID_unlockEvts();
        return (ISI_RETURN_INVALID_TEL_EVENT_ID);
    }

    t_ptr = &evt_ptr->isiMsg.msg.event;
    if (t_ptr->evt == ISI_TEL_EVENT_INVALID) {
        /* It's not valid */
        ISID_unlockEvts();
        return (ISI_RETURN_FAILED);
    }
    *telEvent_ptr = t_ptr->evt;

    ISID_evtDestroy(evtId);
    
    ISID_unlockEvts();
    
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_startConfCall() ========
 * This function is used to start a conference between two calls. Calls in
 * a conference may use different services. Calls must be active before they can
 * be added to the conference call. A 3-way call is started when this function
 * is called.  To add another participant to have a 4-way call, applications
 * can use ISI_addCallToConf(). Once a conference call is started, it will NOT
 * end until all calls participating in the conference are removed via
 * ISI_removeCallFromConf()
 *
 * The party that establishes the conference call acts as a conference bridge.
 * If a conference is established by the local agent, it will be responsible for
 * summing and distributing media streams.  Applications must make sure that the
 * local endpoint has enough resources to accomodate all the parties in the
 * conference bridge.
 *
 * The following rules apply to conference calling:
 *  1. A conference cannot be created between calls of different media types.
 *     For example, video call cannot be added to a conference between audio
 *     calls.
 *  2. Calls from different services can be added to a conference.
 *  3. ISI allows audio and text conferencing only.
 *  4. Conference can be started when two or more calls are in
 *     ISI_CALL_STATE_ACTIVE or ISI_CALL_STATE_ONHOLD state.
 *  5. When no calls are left in a conference, the conference ID related to the
 *    conference becomes invalid and the conference is considered terminated.
 *
 * confId_ptr : A pointer to a memory location where ISI will write unique
 *              identification number representing this conference.
 *              This unique ID is used in future API calls related to call
 *              conferencing.
 *
 * callId0 : A call identification number of the call to be joined in
 *           conference.
 *
 * callId1 : A call identification number to a second call to be joined in
 *           conference.
 *
 * Return Values:
 * ISI_RETURN_OK              : Conference started.
 * ISI_RETURN_FAILED          : Conference call cannot be started.
 * ISI_RETURN_INVALID_CALL_ID : Call IDs of one or both calls are invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Conference linked list.
*/
ISI_Return ISI_startConfCall(
    ISI_Id *confId_ptr,
    ISI_Id  callId0,
    ISI_Id  callId1)
{
    ISID_CallId  *c0_ptr;
    ISID_CallId  *c1_ptr;
    ISID_ConfId  *conf_ptr;
    ISI_Id        confId;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!confId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    if (ISID_lockConfs() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Verify that these calls exist and are in the correct state */
    if (ISID_callGet(callId0, &c0_ptr) != ISI_RETURN_OK) {
        /* Call doesn't exist */
        ISID_unlockConfs();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    if (ISID_callGet(callId1, &c1_ptr) != ISI_RETURN_OK) {
        /* Call doesn't exist */
        ISID_unlockConfs();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    if (c0_ptr->state != ISI_CALL_STATE_ONHOLD &&
            c0_ptr->state != ISI_CALL_STATE_ACTIVE) {
        ISID_unlockConfs();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    if (c1_ptr->state != ISI_CALL_STATE_ONHOLD &&
            c1_ptr->state != ISI_CALL_STATE_ACTIVE) {
        ISID_unlockConfs();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    /* Conferenced calls MUST share a same type that its not chat */
    if (!((c0_ptr->type & c1_ptr->type) & (~ISI_SESSION_TYPE_CHAT))) {
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }
    /* Create/alloc an internal object for use */
    if ((conf_ptr = ISID_confCreate()) == NULL) {
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }

    /* Set the session type of the conference */
    conf_ptr->type = c0_ptr->type;

    if (ISID_confAddCall(conf_ptr, c0_ptr) != ISI_RETURN_OK) {
        ISI_free(conf_ptr, ISI_OBJECT_CONF_ID);
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }
    if (ISID_confAddCall(conf_ptr, c1_ptr) != ISI_RETURN_OK) {
        /* Then we failed to load both, we exceeded the limit */
        ISI_free(conf_ptr, ISI_OBJECT_CONF_ID);
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }

    /* Insert the conf object into the database */
    confId = 0;
    if (ISID_confAdd(conf_ptr, &confId) != ISI_RETURN_OK) {
        /* We exceeded the total number of allowed conferences in the system */
        ISI_free(conf_ptr, ISI_OBJECT_CONF_ID);
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }
    if ((msg_ptr = ISIM_conf(conf_ptr, ISIP_MEDIA_REASON_CONFSTART)) == NULL) {
        ISID_confDestroy(confId);
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }
    if (ISIF_appMsg(conf_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_confDestroy(confId);
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }
    /* Give the id handle to the caller */
    *confId_ptr = confId;
    ISID_unlockConfs();
    return (ISI_RETURN_OK);
}


/*
 * ======== ISI_addCallToConf() ========
 * This function is used to add a call to a conference call.  Calls may or may
 * not use same service.
 *
 * confId : a unique identification number representing this conference.  This
 *          unique ID was returned by ISI when ISI_startConfCall() was called.
 *
 * callId : A call identification number of the call to be joined in
 *          conference.
 *
 * Return Values:
 * ISI_RETURN_OK              : Call added to conference.
 * ISI_RETURN_FAILED          : Call cannot be added to conference.
 * ISI_RETURN_INVALID_CONF_ID : Conference ID is invalid.
 * ISI_RETURN_INVALID_CALL_ID : Call IDs is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Conference linked list.
*/
ISI_Return ISI_addCallToConf(
    ISI_Id confId,
    ISI_Id callId)
{
    ISID_CallId  *call_ptr;
    ISID_ConfId  *conf_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (ISID_lockConfs() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Verify that this conference exists */
    if (ISID_confGet(confId, &conf_ptr) != ISI_RETURN_OK) {
        /* conf doesn't exist */
        ISID_unlockConfs();
        return (ISI_RETURN_INVALID_CONF_ID);
    }
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Call doesn't exist */
        ISID_unlockConfs();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    if (call_ptr->state != ISI_CALL_STATE_ONHOLD &&
            call_ptr->state != ISI_CALL_STATE_ACTIVE) {
        ISID_unlockConfs();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    /* Conferenced calls MUST share a same type that its not chat */
    if (!((call_ptr->type & conf_ptr->type) & (~ISI_SESSION_TYPE_CHAT))) {
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }

    if (ISID_confAddCall(conf_ptr, call_ptr) != ISI_RETURN_OK) {
        /* Then we failed to load it, no room */
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }
    if ((msg_ptr = ISIM_conf(conf_ptr, ISIP_MEDIA_REASON_CONFSTART)) == NULL) {
        /* Remove it from the list */
        ISID_confRemoveCall(conf_ptr, call_ptr, 1);
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }
    if (ISIF_appMsg(conf_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_confRemoveCall(conf_ptr, call_ptr, 1);
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockConfs();
    return (ISI_RETURN_OK);
}


/*
 * ======== ISI_removeCallFromConf() ========
 * This function is used to remove a call from a conference call.
 * When all calls are removed from a conferenced call, the conference is
 * considered dead and the confId is no longer valid. Further references to the
 * conference will return ISI_RETURN_INVALID_CONF_ID.
 *
 * confId : A unique identification number representing the conference.  This
 *          unique ID was returned by ISI when ISI_startConfCall() was called.
 *
 * callId : The call identification number of the call to be removed from
 *          conference.
 *
 * Return Values:
 * ISI_RETURN_OK              : Call removed from conference.
 * ISI_RETURN_FAILED          : Call cannot be removed from conference.
 * ISI_RETURN_INVALID_CONF_ID : Conference ID is invalid.
 * ISI_RETURN_INVALID_CALL_ID : Call IDs is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Conference linked list.
 */
ISI_Return ISI_removeCallFromConf(
    ISI_Id confId,
    ISI_Id callId)
{
    ISID_CallId  *call_ptr;
    ISID_ConfId  *conf_ptr;
    ISIP_Message *msg_ptr;
    ISI_Return    ret;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (ISID_lockConfs() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Verify that this conference exists */
    if (ISID_confGet(confId, &conf_ptr) != ISI_RETURN_OK) {
        /* conf doesn't exist */
        ISID_unlockConfs();
        return (ISI_RETURN_INVALID_CONF_ID);
    }
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Call doesn't exist */
        ISID_unlockConfs();
        return (ISI_RETURN_INVALID_CALL_ID);
    }
    ret = ISID_confRemoveCall(conf_ptr, call_ptr, 0);
    if (ret == ISI_RETURN_FAILED) {
        /* Then it didn't exist  */
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }
    if ((msg_ptr = ISIM_conf(conf_ptr, ISIP_MEDIA_REASON_CONFSTOP)) == NULL) {
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }
    if (ISIF_appMsg(conf_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_unlockConfs();
        return (ISI_RETURN_FAILED);
    }
    if (ret == ISI_RETURN_DONE) {
        /* Then there are no more active calls, so kill this conference */
        ISID_confDestroy(confId);
    }
    ISID_unlockConfs();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_sendMessage ========
 * Applications use this function to send a message to a remote agent.
 * The message being sent will NOT be associated with any particular chat
 * session or call.
 *
 * When the message is delivered to the remote agent, ISI will generate a
 * ISI_EVENT_MESSAGE_SEND_OK event signifying that the message was delivered.
 * If the message could not be delivered, ISI will generate a
 * ISI_EVENT_MESSAGE_SEND_FAILED event.
 *
 * msgId_ptr : A pointer to memory location where ISI will write a unique
 *             identification number representing this message transaction.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called
 *
 * type : The type of message being send.  Text, PDU 3GPP, or PDU 3GPP2
 *
 * to_ptr : A null terminated string containing the address(s) of the remote
 *          agent to send the message to.
 *
 * subject_ptr : A pointer to memory location that has a null terminated
 *               string containing subject of call.  If subject_ptr is set to
 *               null, no subject will be include with the message.
 *
 * msg_ptr : A null terminated string containing the text body of the message.
 *           In other words this is the actual text message to send.
 *
 * msgLen : the length of the binary message being sent.  Ignore for Text type messages.
 *
 * report : An optional parameter containing a bitmap of reports to request
 *          (i.e. 'delivery' or 'display' reports).
 *
 * reportId_ptr : An optional parameter containing a report ID to associate with this
 *                instant message.
 *
 * Return Values:
 * ISI_RETURN_OK              : The message transaction has successfully
 *                              started.
 * ISI_RETURN_FAILED          : Function failed.  The message was not sent.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Text Message linked list.
 */
ISI_Return ISI_sendMessage(
    ISI_Id            *msgId_ptr,
    ISI_Id             serviceId,
    ISI_MessageType    type,
    char              *to_ptr,
    char              *subject_ptr,
    char              *msg_ptr,
    int                msgLen,
    ISI_MessageReport  report,
    char              *reportId_ptr)
{
    ISID_ServiceId  *service_ptr;
    ISID_TextId     *text_ptr;
    ISIP_Text       *t_ptr;
    ISI_Return       ret;
    vint             len;
    ISI_Id           textId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!msgId_ptr || !to_ptr || !msg_ptr || !subject_ptr) {
        return (ISI_RETURN_FAILED);
    }

    if (report != ISI_MSG_RPT_NONE) {
        /* Then let's validate the id_ptr */
        if (!reportId_ptr) {
            return (ISI_RETURN_FAILED);
        }
    }

    /* Lock the database of texts */
    if (ISID_lockTexts() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    ret = ISID_serviceGet(serviceId, &service_ptr);

    if (ret == ISI_RETURN_FAILED) {
        /* Then there is no service */
        ISID_unlockTexts();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }
    else if (ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
        /* Then the service is not active */
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }
    else {
        /* The service Id is valid and the service is active */
        if ((text_ptr = ISID_textCreate()) == NULL) {
            /* no available resources */
            ISID_unlockTexts();
            return (ISI_RETURN_FAILED);
        }
        t_ptr = &text_ptr->isiMsg.msg.message;

        /* Set the type */
        t_ptr->type = type;

        /* Copy the message */
        if (ISI_MSG_TYPE_TEXT == type) {
            /* Text type */
            len = OSAL_strlen(msg_ptr);
            if (len >= (vint)sizeof(t_ptr->message)) {
                ISI_free(text_ptr, ISI_OBJECT_TEXT_ID);
                ISID_unlockTexts();
                return (ISI_RETURN_FAILED);
            }
            OSAL_strncpy(t_ptr->message, msg_ptr, sizeof(t_ptr->message));
            t_ptr->pduLen = len;
            text_ptr->msgLen = len;
        }
        else {
            /* else PDU type */
            if (msgLen > ISI_TEXT_STRING_SZ) {
                ISI_free(text_ptr, ISI_OBJECT_TEXT_ID);
                ISID_unlockTexts();
                return (ISI_RETURN_FAILED);
            }
            OSAL_memCpy(t_ptr->message, msg_ptr, msgLen);
            t_ptr->pduLen = msgLen;
            text_ptr->msgLen = msgLen;
        }
        /* Copy the rest */
        text_ptr->service_ptr = service_ptr;
        OSAL_strncpy(t_ptr->to, to_ptr, ISI_LONG_ADDRESS_STRING_SZ);
        OSAL_strncpy(t_ptr->subject, subject_ptr, ISI_SUBJECT_STRING_SZ);
        /* Copy the reports that we are requesting for this text message */
        t_ptr->report = report;
        if (report != ISI_MSG_RPT_NONE) {
            /* Then we have an id value, so let's populate */
            OSAL_strncpy(t_ptr->messageId, reportId_ptr, ISI_ID_STRING_SZ);
        }
        
        /* Add it to the database.  Make sure msgId_ptr is NULL */
        textId = 0;
        if (ISID_textAdd(text_ptr, &textId) != ISI_RETURN_OK) {
            ISI_free(text_ptr, ISI_OBJECT_TEXT_ID);
            ISID_unlockTexts();
            return (ISI_RETURN_FAILED);
        }
        /* Populate an ISI Message and send */
        ISIM_sendText(ISIP_TEXT_REASON_NEW, text_ptr, serviceId, 0);
        ISIT_appMsg(text_ptr, &text_ptr->isiMsg);
        *msgId_ptr = textId;
        ISID_unlockTexts();
        return (ISI_RETURN_OK);
    }
}

/*
 * ======== ISI_sendMessageReport ========
 * Applications use this function to send a report for a message
 * when the remote party requested a report. For example let's say this
 * endpoint recevied a text message and the remote party requested a
 * 'devivery' (via the 'ISI_MSG_RPT_DELIVERY_SUCCESS' report value).
 * The application could then use this routine to send back the
 * successful delivery report.
 *
 * ISI will generate a ISI_EVENT_MESSAGE_SEND_OK or ISI_EVENT_MESSAGE_SEND_FAILED
 * event indicating if the report was successfully sent or not.
 *
 * msgId_ptr : A pointer to memory location where ISI will write a unique
 *             identification number representing this message transaction.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called
 *
 * to_ptr : A null terminated string containing the address(s) of the remote
 *          agent to send the report to.
 *
 * report : A value representing the report being sent.  See ISI_MessageReport
 *          definition for all possible values.
 *
 * reportId_ptr : A null terminated string containing a ID value used to match
 *                a text message and associated reports.  This should be
 *                the same reportId that was received when the original text message
 *                was received.
 *
 * Return Values:
 * ISI_RETURN_OK              : The message transaction has successfully
 *                              started.
 * ISI_RETURN_FAILED          : Function failed.  The message was not sent.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Text Message linked list.
 */
ISI_Return ISI_sendMessageReport(
    ISI_Id                  *msgId_ptr,
    ISI_Id                   serviceId,
    char                    *to_ptr,
    ISI_MessageReport        report,
    char                    *reportId_ptr)
{
    ISID_ServiceId  *service_ptr;
    ISID_TextId     *text_ptr;
    ISIP_Text       *t_ptr;
    ISI_Return       ret;
    ISI_Id           textId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!msgId_ptr || !to_ptr || !reportId_ptr || ISI_MSG_RPT_NONE == report) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of texts */
    if (ISID_lockTexts() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    ret = ISID_serviceGet(serviceId, &service_ptr);

    if (ret == ISI_RETURN_FAILED) {
        /* Then there is no service */
        ISID_unlockTexts();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }
    else if (ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
        /* Then the service is not active */
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }
    else {
        /* The service Id is valid and the service is active */
        if ((text_ptr = ISID_textCreate()) == NULL) {
            /* no available resources */
            ISID_unlockTexts();
            return (ISI_RETURN_FAILED);
        }
        t_ptr = &text_ptr->isiMsg.msg.message;
        text_ptr->service_ptr = service_ptr;
        OSAL_strncpy(t_ptr->to, to_ptr, ISI_ADDRESS_STRING_SZ);
        t_ptr->report = report;
        OSAL_strncpy(t_ptr->messageId, reportId_ptr, ISI_ID_STRING_SZ);
        
        /* Add it to the database.  Make sure msgId_ptr is NULL */
        textId = 0;
        if (ISID_textAdd(text_ptr, &textId) != ISI_RETURN_OK) {
            ISI_free(text_ptr, ISI_OBJECT_TEXT_ID);
            ISID_unlockTexts();
            return (ISI_RETURN_FAILED);
        }
        /* Populate an ISI Message and send */
        ISIM_sendText(ISIP_TEXT_REASON_REPORT, text_ptr, serviceId, 0);
        ISIT_appMsg(text_ptr, &text_ptr->isiMsg);
        *msgId_ptr = textId;
        ISID_unlockTexts();
        return (ISI_RETURN_OK);
    }
}

ISI_Return ISI_readMessageReport(
    ISI_Id             msgId,
    ISI_Id            *chatId_ptr,
    char              *from_ptr,
    char              *dateTime_ptr,
    ISI_MessageReport *report_ptr,
    char              *reportId_ptr)
{
    ISID_TextId  *text_ptr;
    ISIP_Text    *t_ptr;
    
    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!chatId_ptr || !report_ptr || !reportId_ptr ||
            !dateTime_ptr || !from_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of texts */
    if (ISID_lockTexts() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the message via the messageId */
    if (ISID_textGet(msgId, &text_ptr) != ISI_RETURN_OK) {
        ISID_unlockTexts();
        return (ISI_RETURN_INVALID_MESSAGE_ID);
    }
    *chatId_ptr = text_ptr->chatId;
    t_ptr = &text_ptr->isiMsg.msg.message;
    *report_ptr = t_ptr->report;
    OSAL_strncpy(reportId_ptr, t_ptr->messageId, ISI_ID_STRING_SZ);
    OSAL_strncpy(dateTime_ptr, t_ptr->dateTime, ISI_DATE_TIME_STRING_SZ);
    OSAL_strncpy(from_ptr, t_ptr->from, ISI_ADDRESS_STRING_SZ);

    /* The whole message has been written so free up the text resource */
    ISID_textDestroy(msgId);
    ISID_unlockTexts();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_getMessageHeader() ========
 * Applications use this function to receive header information of a text
 * message that has been received. The message header is a combination of the
 * message subject and the sender's address.
 *
 * ISI notifies applications of new incoming text messages via
 * the ISI_EVENT_MESSAGE_RECEIVED event.  Applications can call this function
 * to read the header information of the message.
 *
 * NOTE: To receive header information of a message received within a chat
 * session/call, applications must use function ISI_getCallHeader() instead.
 *
 * msgId : The identification number representing the message for which to
 *         retrieve the message header info.
 *
 * type : The ISI_MessageType of the incoming Message.
 *
 * subject_ptr : A pointer to memory location where ISI will write a null
 *               terminated string containing 'subject' of a message. If there
 *               is no subject included with the message, then this routine
 *               will copy an empty string to subject_ptr. Note that this buffer
 *               must be at least ISI_SUBJECT_STRING_SZ bytes in size.
 *
 * from_ptr : A pointer to memory location where ISI will write a null
 *            terminated string containing the address of sender.  Note that
 *            this buffer must be at least ISI_ADDRESS_STRING_SZ bytes in size.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Message header returned.
 * ISI_RETURN_FAILED             : Message header cannot be returned.
 * ISI_RETURN_INVALID_MESSAGE_ID : Message ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Text Message linked list.
 */
ISI_Return ISI_getMessageHeader(
    ISI_Id                   msgId,
    ISI_OUT ISI_MessageType *type_ptr,
    char                    *subject_ptr,
    char                    *from_ptr,
    char                    *dateTime_ptr,
    ISI_MessageReport       *reports_ptr,
    char                    *reportId_ptr)
{
    ISID_TextId *text_ptr;
    ISIP_Text   *t_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!subject_ptr || !from_ptr || !dateTime_ptr ||
            !reports_ptr || !reportId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of texts */
    if (ISID_lockTexts() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_textGet(msgId, &text_ptr) != ISI_RETURN_OK) {
        ISID_unlockTexts();
        return (ISI_RETURN_INVALID_MESSAGE_ID);
    }

    t_ptr = &text_ptr->isiMsg.msg.message;

    if (t_ptr->from[0] == 0) {
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }
    /* Everything is okay so copy to the target */
    *type_ptr = t_ptr->type;
    OSAL_strncpy(from_ptr, t_ptr->from, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(subject_ptr, t_ptr->subject, ISI_SUBJECT_STRING_SZ);
    OSAL_strncpy(dateTime_ptr, t_ptr->dateTime, ISI_DATE_TIME_STRING_SZ);
    OSAL_strncpy(reportId_ptr, t_ptr->messageId, ISI_ID_STRING_SZ);
    *reports_ptr = t_ptr->report;
    ISID_unlockTexts();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_readMessage() ========
 * Applications use this function to read a message that arrived from a remote
 * agent. ISI will notify the application of a new incoming message via the
 * ISI_EVENT_MESSAGE_RECEIVED event. After receiving this event, applications
 * call this function to retrieve the message from ISI.
 *
 * Application must call this function repeatedly to receive complete message
 * from ISI. Message is completely written when function returns
 * ISI_RETURN_DONE.
 *
 * msgId : An identification number representing the message to retrieve.
 *
 * chatId_ptr : a pointer to memory location where ISI will write the group chat
 *              identification number for the chat room in whose context this
 *              message was received. If the message was received outside a
 *              group chat room then ISI will write a '0' to this memory
 *              location.
 *
 * part_ptr : a pointer to the memory location where ISI will write the message
 *            (or part of the message).
 *
 * len_ptr : (i/o) The length of the buffer pointed to by part_ptr. If the length is
 *       shorter than the length of the received message, ISI will only write
 *       up to this length and then return ISI_RETURN_OK.  The output value of
 *       len_ptr is the actual number of bytes/chars read into part_ptr buffer.
 *       It is up to the application to repeatedly call this function until ISI_RETURN_DONE
 *       is returned indicating that the complete message has been read.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Part of the message has been written to
 *                                 part_ptr.
 * ISI_RETURN_FAILED             : Function failed.  Invalid parameters.
 * ISI_RETURN_DONE               : The function is successfully and the entire
 *                                 text message has been read by the
 *                                 application.
 * ISI_RETURN_INVALID_MESSAGE_ID : The message ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI is not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Text Message linked list.
 */
ISI_Return ISI_readMessage(
    ISI_Id  msgId,
    ISI_Id *chatId_ptr,
    char   *part_ptr,
    int    *len_ptr)
{
    ISID_TextId  *text_ptr;
    ISIP_Text    *t_ptr;
    vint          strLen;
    char         *str_ptr;
    vint          len;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!chatId_ptr || !part_ptr || 0 >= *len_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of texts */
    if (ISID_lockTexts() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the message via the messageId */
    if (ISID_textGet(msgId, &text_ptr) != ISI_RETURN_OK) {
        ISID_unlockTexts();
        return (ISI_RETURN_INVALID_MESSAGE_ID);
    }
    *chatId_ptr = text_ptr->chatId;

    t_ptr = &text_ptr->isiMsg.msg.message;

    if (ISI_MSG_TYPE_TEXT == t_ptr->type) {
        len = *len_ptr;
        /*  Text string type. */
        str_ptr = t_ptr->message + text_ptr->msgOffset;
        strLen  = text_ptr->msgLen - text_ptr->msgOffset;
        OSAL_strncpy(part_ptr, str_ptr, len);
        if (strLen >= len) {
            /*
             * Then the output buffer was too short for the whole thing with
             * a NULL termination. Update the position in the from buffer
             */
            len--;
            *len_ptr = len;
            text_ptr->msgOffset += len;
            ISID_unlockTexts();
            return (ISI_RETURN_OK);
        }
        *len_ptr = strLen;  /*  save the actual length for output */
    }
    else {
        /* PDU types */
        /* Copy the binary data */
        OSAL_memCpy(part_ptr,  t_ptr->message, t_ptr->pduLen);
        /* Output the binary length */
        *len_ptr = t_ptr->pduLen;
    }

    /* The whole message has been written so free up the text resource */
    ISID_textDestroy(msgId);
    ISID_unlockTexts();
    return (ISI_RETURN_DONE);
}

/*
 * ======== ISI_sendFile ========
 * Applications use this function to send a file to a remote agent.
 * The file being sent will NOT be associated with any particular group chat
 * session.
 *
 * During the file transfer, ISI may generate any of the following events:
 * ISI_EVENT_FILE_SEND_PROGRESS: Indicates a progress report of the file transfer.
 * ISI_EVENT_FILE_SEND_PROGRESS_COMPLETED: Indicates that the file successfully completed transferring.
 * ISI_EVENT_FILE_SEND_PROGRESS_FAILED: The file transfer failed to complete.
 *
 * fileId_ptr : A pointer to memory location where ISI will write a unique
 *             identification number representing this file transfer transaction.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called
 *
 * to_ptr : A null terminated string containing the address of the remote agent
 *          to send the file to.
 *
 * subject_ptr : A pointer to memory location that has a null terminated
 *               string containing subject of transfer.  If subject_ptr is set
 *               to null, no subject will be include with the message.
 *
 * filePath_ptr : A null terminated string containing the absolute path of the
 *                file to send.
 *
 * fileType : The type of file being sent.  See the definintion of ISI_FileType
 *           for more details.
 *
 * attribute : The attributes
 *
 * report : The file transfer request report
 * Return Values:
 * ISI_RETURN_OK              : The file transaction transaction has 
 *                              successfully started.
 * ISI_RETURN_FAILED          : Function failed.  The file was not sent.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the File Transfer linked list.
 */
ISI_Return ISI_sendFile(
    ISI_Id            *fileId_ptr,
    ISI_Id             serviceId,
    char              *to_ptr,
    char              *subject_ptr,
    char              *filePath_ptr,
    ISI_FileType       fileType,
    ISI_FileAttribute  attribute,
    int                fileSize,
    ISI_FileReport     report)
{
    ISID_ServiceId  *service_ptr;
    ISID_FileId     *file_ptr;
    ISIP_File       *f_ptr;
    ISI_Return       ret;
    vint             len;
    ISI_Id           fileId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!fileId_ptr || !to_ptr || !filePath_ptr) {
        return (ISI_RETURN_FAILED);
    }

    if (ISI_FILETYPE_INVALID <= fileType) {
        return (ISI_RETURN_FAILED);
    }

    len = OSAL_strlen(filePath_ptr);
    if (len >= ISI_FILE_PATH_STRING_SZ) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of texts */
    if (ISID_lockFiles() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    ret = ISID_serviceGet(serviceId, &service_ptr);

    if (ret == ISI_RETURN_FAILED) {
        /* Then there is no service */
        ISID_unlockFiles();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }
    else if (ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
        /* Then the service is not active */
        ISID_unlockFiles();
        return (ISI_RETURN_FAILED);
    }
    else {
        /* The service Id is valid and the service is active */
        if ((file_ptr = ISID_fileCreate()) == NULL) {
            /* no available resources */
            ISID_unlockFiles();
            return (ISI_RETURN_FAILED);
        }
        /* Copy the rest */
        file_ptr->service_ptr = service_ptr;
        file_ptr->state = ISID_FILE_STATE_SEND;

        /* Build up the bulk of the isi msg to send */
        f_ptr = &file_ptr->isiMsg.msg.file;
        f_ptr->chatId = 0;
        f_ptr->fileType = fileType;
        f_ptr->fileAttribute = attribute;
        f_ptr->report = report;
      
        OSAL_strncpy(f_ptr->filePath, filePath_ptr, sizeof(f_ptr->filePath));
        /* Return if file size = 0 */
        if (0 >= fileSize) {
            ISID_unlockFiles();
            return (ISI_RETURN_FAILED);
        }
        f_ptr->fileSize = fileSize;
        OSAL_strncpy(f_ptr->to, to_ptr, ISI_ADDRESS_STRING_SZ);
        OSAL_strncpy(f_ptr->from, service_ptr->szUri, ISI_ADDRESS_STRING_SZ);
        if (NULL != subject_ptr) {
            OSAL_strncpy(f_ptr->subject, subject_ptr, ISI_SUBJECT_STRING_SZ);
        }
        /* Add it to the database. */
        fileId = 0;
        if (ISID_fileAdd(file_ptr, &fileId) != ISI_RETURN_OK) {
            ISI_free(file_ptr, ISI_OBJECT_FILE_ID);
            ISID_unlockFiles();
            return (ISI_RETURN_FAILED);
        }
        /* Populate the rest of the ISI Message and send */
        ISIM_sendFile(file_ptr, serviceId);
        ISIFT_appMsg(file_ptr, &file_ptr->isiMsg);
        *fileId_ptr = fileId;
        ISID_unlockFiles();
        return (ISI_RETURN_OK);
    }
}


/*
 * ======== ISI_acceptFile ========
 * Applications use this function to accept a file transfer request
 * received from a remote agent.
 *
 * fileId : A unique identification number representing this file transfer
 *          transaction.
 */
ISI_Return ISI_acceptFile(ISI_Id fileId) {
    ISID_FileId  *file_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of texts */
    if (ISID_lockFiles() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the message via the messageId */
    if (ISID_fileGet(fileId, &file_ptr) != ISI_RETURN_OK) {
        ISID_unlockFiles();
        return (ISI_RETURN_INVALID_FILE_ID);
    }

    /* Everything is okay so notify the protocol */
    /* Populate the rest of the ISI Message and send */
    ISIM_acceptFileTransfer(file_ptr);
    ISIFT_appMsg(file_ptr, &file_ptr->isiMsg);

    ISID_unlockFiles();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_acknowledgeFile ========
 * Applications use this function to acknowledge that a file transfer request
 * was received from a remote agent.  It will notify the protocol which may,
 * for example, send a provisional response (e.g.-180 Ringing, in the case
 * of SAPP/SIMPLE)
 *
 * fileId : A unique identification number representing this file transfer
 *          transaction.
 */
ISI_Return ISI_acknowledgeFile(ISI_Id fileId) {
    ISID_FileId  *file_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of texts */
    if (ISID_lockFiles() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the message via the messageId */
    if (ISID_fileGet(fileId, &file_ptr) != ISI_RETURN_OK) {
        ISID_unlockFiles();
        return (ISI_RETURN_INVALID_FILE_ID);
    }

    /* Everything is okay so notify the protocol */
    /* Populate the rest of the ISI Message and send */
    ISIM_acknowledgeFileTransfer(file_ptr);
    ISIFT_appMsg(file_ptr, &file_ptr->isiMsg);

    ISID_unlockFiles();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_rejectFile ========
 * Applications use this function to reject a file transfer request
 * received from a remote agent.
 *
 * fileId : A unique identification number representing this file transfer
 *          transaction.
 *
 * reason_ptr : An optional parameter that can contain a null terminated
 *              string describing the reason why the invitation was rejected.
 */
ISI_Return ISI_rejectFile(ISI_Id fileId, const char *reason_ptr) {
    ISID_FileId  *file_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of texts */
    if (ISID_lockFiles() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the message via the messageId */
    if (ISID_fileGet(fileId, &file_ptr) != ISI_RETURN_OK) {
        ISID_unlockFiles();
        return (ISI_RETURN_INVALID_FILE_ID);
    }

    /* Everything is okay so notify the protocol */
    /* Populate the rest of the ISI Message and send */
    ISIM_rejectFileTransfer(file_ptr, reason_ptr);
    ISIFT_appMsg(file_ptr, &file_ptr->isiMsg);

    ISID_unlockFiles();
    return (ISI_RETURN_OK);
}


/*
 * ======== ISI_cancelFile ========
 * Applications use this function to cancel a file transfer request
 * that is previously sent to a remote agent.
 *
 * fileId : A unique identification number representing this file transfer
 *          transaction that was obtained via ISI_sendFile()
 *
 * reason_ptr : An optional parameter that can contain a null terminated
 *              string describing the reason why the invitation was rejected.
 */
ISI_Return ISI_cancelFile(ISI_Id fileId, const char *reason_ptr) {
    ISID_FileId  *file_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of texts */
    if (ISID_lockFiles() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the message via the messageId */
    if (ISID_fileGet(fileId, &file_ptr) != ISI_RETURN_OK) {
        ISID_unlockFiles();
        return (ISI_RETURN_INVALID_FILE_ID);
    }

    /* Everything is okay so notify the protocol */
    /* Populate the rest of the ISI Message and send */
    ISIM_cancelFileTransfer(file_ptr, reason_ptr);
    ISIFT_appMsg(file_ptr, &file_ptr->isiMsg);

    ISID_unlockFiles();
    return (ISI_RETURN_OK);
}


/*
 * ======== ISI_readFileProgress() ========
 * Applications use this function to read the progress if a file transfer.
 * ISI will notify the application of file transfer progress via the following
 * events:
 * ISI_EVENT_FILE_SEND_PROGRESS
 * ISI_EVENT_FILE_SEND_PROGRESS_COMPLETED
 * ISI_EVENT_FILE_SEND_PROGRESS_FAILED
 * ISI_EVENT_FILE_RECV_PROGRESS
 * ISI_EVENT_FILE_RECV_PROGRESS_COMPLETED
 * ISI_EVENT_FILE_RECV_PROGRESS_FAILED
 *
 * After receiving any of these events, applications can call this function to
 * retrieve the details of the progress update.  Note that the progress is
 * reported as a percentage of completion.  In otherwords, if the progress
 * is '80' then 80% of the download/upload is completed
 *
 * fileId : An identification number representing the file transfer of interest.
 *
 * chatId_ptr : a pointer to memory location where ISI will write the group chat
 *              identification number for the chat room in whose context this
 *              file transfer is happening within. If the file transfer is
 *              happening outside the context of the group chat then ISI will
 *              write a '0' to this memory location.
 *
 * filePath_ptr : a pointer to the memory location where ISI will write the
 *                filepath of the file being downloaded.  Note that this buffer
 *                must be at least ISI_FILE_PATH_STRING_SZ bytes in size.
 *
 * fileType_ptr : a pointer to the memory location where ISI will write an
 *                enumerated value representing the type of file being
 *                downloaded.
 *
 * progress_ptr : a pointer to the memory location where ISI will write the
 *                progress of the file transfer.  This value will be a
 *                percentage completed of the transfer.  For example,
 *                if ISI writes an '80' here, then 80% of the file transfer
 *                is completed.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Progress was successfully reported. The value
 *                                 in the progress_ptr will be the percentage of
 *                                 the file transfer that has completed.
 * ISI_RETURN_FAILED             : Function failed.  Invalid parameters.
 * ISI_RETURN_DONE               : Progress was successfully reported and this
 *                                 report will be THE LAST ONE.  In otherwords
 *                                 the transaction has completed.
 * ISI_RETURN_INVALID_FILE_ID    : The file ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI is not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the File Transfer linked list.
 */
ISI_Return ISI_readFileProgress(
    ISI_Id               fileId,
    /*ISI_Id       *chatId_ptr,*/
    char                *filePath_ptr,
    ISI_FileType        *fileType_ptr,
    ISI_FileAttribute   *fileAttribute_ptr,
    vint                *fileSize_ptr,
    vint                *progress_ptr)
{
    ISID_FileId  *file_ptr;
    ISIP_File    *f_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (/*!chatId_ptr ||*/ !filePath_ptr || !fileType_ptr || !progress_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of texts */
    if (ISID_lockFiles() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the message via the messageId */
    if (ISID_fileGet(fileId, &file_ptr) != ISI_RETURN_OK) {
        ISID_unlockFiles();
        return (ISI_RETURN_INVALID_FILE_ID);
    }


    f_ptr = &file_ptr->isiMsg.msg.file;

    /* Everything is okay so copy to the target */
    OSAL_strncpy(filePath_ptr, f_ptr->filePath, ISI_FILE_PATH_STRING_SZ);
    *fileType_ptr = f_ptr->fileType;
    *progress_ptr = f_ptr->progress;
    /* *chatId_ptr = f_ptr->chatId; */
    *fileAttribute_ptr = f_ptr->fileAttribute;
    *fileSize_ptr = f_ptr->fileSize;
        

    ISID_unlockFiles();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_getFileHeader() ========
 * Applications use this function to receive header information of a file
 * transfer that is in progress.
 *
 * ISI notifies applications of file transfer progress via the following:
 * ISI_EVENT_FILE_SEND_PROGRESS
 * ISI_EVENT_FILE_SEND_PROGRESS_COMPLETED
 * ISI_EVENT_FILE_SEND_PROGRESS_FAILED
 * ISI_EVENT_FILE_RECV_PROGRESS
 * ISI_EVENT_FILE_RECV_PROGRESS_COMPLETED
 * ISI_EVENT_FILE_RECV_PROGRESS_FAILED
 *
 * Applications can call this function to read the header information of the
 * file transfer in progress.
 *
 * fileId : The identification number representing the file transfer
 *          transaction.
 *
 * subject_ptr : A pointer to memory location where ISI will write a null
 *               terminated string containing 'subject' of a file transfer.
 *               If there is no 'subject' associated with this fiel transfer,
 *               then this routine will copy an empty string to subject_ptr.
 *               Note that this buffer must be at least ISI_SUBJECT_STRING_SZ
 *               bytes in size.
 *
 * from_ptr : A pointer to memory location where ISI will write a null
 *            terminated string containing the address of sender.  Note that
 *            this buffer must be at least ISI_ADDRESS_STRING_SZ bytes in size.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Message header returned.
 * ISI_RETURN_FAILED             : Message header cannot be returned.
 * ISI_RETURN_INVALID_FILE_ID    : Message ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the File Transfer linked list.
 */
ISI_Return ISI_getFileHeader(
    ISI_Id        fileId,
    char         *subject_ptr,
    char         *from_ptr)
{
    ISID_FileId *file_ptr;
    ISIP_File   *f_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!subject_ptr || !from_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of texts */
    if (ISID_lockFiles() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_fileGet(fileId, &file_ptr) != ISI_RETURN_OK) {
        ISID_unlockFiles();
        return (ISI_RETURN_INVALID_FILE_ID);
    }

    f_ptr = &file_ptr->isiMsg.msg.file;

    /* Everything is okay so copy to the target */
    OSAL_strncpy(from_ptr, f_ptr->from, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(subject_ptr, f_ptr->subject, ISI_SUBJECT_STRING_SZ);

    ISID_unlockFiles();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_addContact() ========
 * This function is used to add a contact (user) to a contact list (roster).
 * Contact lists are managed at the server providing the presence facility.
 * ISI does not maintain, update, or modify any list.  This function issues
 * a request to presence server to add a user to contact list.
 * First argument to this function, presId_ptr, is a pointer to memory
 * location where ISI will write identification number representing the
 * transaction. ISI will notify application of successful addition
 * or a failure using ISI events ISI_EVENT_CONTACT_SEND_OK or
 * ISI_EVENT_CONTACT_SEND_FAILED. Second argument to this function,
 * serviceId, is the service identification number returned by ISI when
 * ISI_allocService() was called.  Third argument to this function, user_ptr,
 * points to a null terminated string containing URI (jid) of user to be added
 * to the list.  Fourth argument to this function, group_ptr, points to a null
 * terminated string containing a "group" in which the user belongs to.
 * Fifth argument to this function, name_ptr, points to a null
 * terminated string containing a name associated to this contact entry.
 * User name and group are optional.
 *
 * presId_ptr : A pointer to memory location where ISI will write an
 *              identification number for this presence transaction.
 *              If this function returns a failure, this value will be invalid.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * user_ptr :  A null terminated string containing URI (jid) of the remote
 *             entity that is being added to the contact list (roster).
 *
 * group_ptr : A null terminated string containing the name of group that
 *             is associated with the entity defined in user_ptr. This is an
 *             optional parameter.
 *
 * name_ptr :  A null terminated string containing a name that
 *             is associated with the entity defined in user_ptr. This is an
 *             optional parameter.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Request sent to server to add a contact.
 * ISI_RETURN_FAILED             : Could not send request to server.
 * ISI_RETURN_INVALID_SERVICE_ID : Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Presence linked list.
 */
ISI_Return ISI_addContact(
    ISI_Id *presId_ptr,
    ISI_Id  serviceId,
    char   *user_ptr,
    char   *group_ptr,
    char   *name_ptr)
{
    ISID_ServiceId  *service_ptr;
    ISID_PresId     *pres_ptr;
    ISIP_Presence   *p_ptr;
    ISI_Id           presId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Check for required parameters */
    if (!presId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    if (!user_ptr || *user_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_OK) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    if ((pres_ptr = ISID_presCreate()) == NULL) {
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }

    p_ptr = &pres_ptr->isiMsg.msg.presence;
    pres_ptr->service_ptr = service_ptr;
    pres_ptr->reason = ISIP_PRES_REASON_CONTACT_ADD;

    /* Add the optional parameters */
    if (group_ptr) {
        OSAL_strncpy(p_ptr->roster.group, group_ptr, ISI_ADDRESS_STRING_SZ);
    }
    if (name_ptr) {
        OSAL_strncpy(p_ptr->roster.name, name_ptr, ISI_ADDRESS_STRING_SZ);
    }

     /* This will force the DB sub module to assign an id */
    presId = 0;
    if (ISID_presAdd(pres_ptr, &presId) == ISI_RETURN_FAILED) {
        /* Free the memory and return */
        ISI_free(pres_ptr, ISI_OBJECT_PRES_ID);
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISI Message and notify the protocol */
    ISIM_pres(pres_ptr, ISIP_PRES_REASON_CONTACT_ADD, user_ptr);
    ISIR_appMsg(pres_ptr, &pres_ptr->isiMsg);
    *presId_ptr = presId;
    ISID_unlockPres();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_removeContact() ========
 * This function is used to remove a contact (roster entry) from a contact
 * list. Contact lists are managed at the server providing presence facility.
 * ISI does not maintain, update, or modify any list.  This function issues a
 * request to presence server to remove a user from contact list (or roster).
 * First argument to this function, presId_ptr, is a pointer to memory location
 * where ISI will write identification number for the contact list update
 * transaction.  ISI will notify application of successful removal or a
 * failure using ISI events ISI_EVENT_CONTACT_SEND_OK or
 * ISI_EVENT_CONTACT_SEND_FAILED with ISI_getEvent() function argument
 * id_ptr set to value returned in presId_ptr.  Second argument to this
 * function, serviceId, is the service identification number returned by ISI
 * when ISI_allocService() was called.  Third argument to this function,
 * user_ptr, points to a null terminated string containing URI of user to be
 * removed from the list.  Fourth argument to this function, group_ptr, points
 * to a null terminated string containing group in which the user to be removed
 * from the list belongs to.
 *
 *
 * presId_ptr : A pointer to memory location where ISI will write an
 *              identification number for this presence transaction.
 *              If this function returns a failure, this value will be invalid.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * user_ptr :  A null terminated string containing URI (jid) of the remote
 *             entity that is being removed from the contact list (roster).
 *
 * group_ptr : A null terminated string containing the name of group that
 *             is associated with the entity defined in user_ptr. This is an
 *             optional parameter.
 *
 * name_ptr :  A null terminated string containing a name that
 *             is associated with the entity defined in user_ptr. This is an
 *             optional parameter.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Request sent to server.
 * ISI_RETURN_FAILED             : Could not send request to server.
 * ISI_RETURN_INVALID_SERVICE_ID : Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Presence linked list.
 */
ISI_Return ISI_removeContact(
    ISI_Id *presId_ptr,
    ISI_Id  serviceId,
    char   *user_ptr,
    char   *group_ptr,
    char   *name_ptr)
{
    ISID_ServiceId  *service_ptr;
    ISIP_Presence   *p_ptr;
    ISID_PresId     *pres_ptr;
    ISI_Id           presId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Check for required parameters */
    if (!presId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    if (!user_ptr || *user_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_OK) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    if ((pres_ptr = ISID_presCreate()) == NULL) {
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }

    p_ptr = &pres_ptr->isiMsg.msg.presence;
    pres_ptr->service_ptr = service_ptr;
    pres_ptr->reason = ISIP_PRES_REASON_CONTACT_RM;

    /* Add the optional parameters */
    if (group_ptr) {
        OSAL_strncpy(p_ptr->roster.group, group_ptr, ISI_ADDRESS_STRING_SZ);
    }
    if (name_ptr) {
        OSAL_strncpy(p_ptr->roster.name, name_ptr, ISI_ADDRESS_STRING_SZ);
    }

    /* This will force the DB sub module to assign an id */
    presId = 0;
    if (ISID_presAdd(pres_ptr, &presId) == ISI_RETURN_FAILED) {
        /* Free the memory and return */
        ISI_free(pres_ptr, ISI_OBJECT_PRES_ID);
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISI Message and notify the protocol */
    ISIM_pres(pres_ptr, ISIP_PRES_REASON_CONTACT_RM, user_ptr);
    ISIR_appMsg(pres_ptr, &pres_ptr->isiMsg);
    *presId_ptr = presId;
    ISID_unlockPres();
    return (ISI_RETURN_OK);
}


/*
 * ======== ISI_readContact() ========
 * Application calls this function to read contact (roster entry) information
 * when the ISI event ISI_EVENT_CONTACT_RECEIVED is received.
 * First argument to this function, presId, contains a unique
 * identification number representing the roster entry being requested.
 * Second argument to this function, user_ptr, is a pointer to memory location
 * where ISI will write the URI (jid) of which this roster entry
 * pertains to. Third argument to this function, group_ptr, is a
 * pointer to memory location where ISI will write the name of the "group"
 * associated with the URI (or jid) specified in user_ptr. This value
 * may or may not be provided pending the underlying signalling protocol.
 * Therefore, developers should consider this value to be optional.
 * Fourth argument to this function, name_ptr, is a pointer to memory location
 * where ISI will write a plain text "name" associated with the URI (or jid)
 * specified in user_ptr. This value may or may not be provided pending the
 * underlying signaling protocol. Therefore, developers should consider this
 * value to be optional.
 *
 * presId    : A unique identification number representing the contact list
 *             entry to retrieve.
 *
 * user_ptr  : A pointer to a buffer where ISI will write a string representing
 *             the remote entity's URI (or jid) of this roster entry.
 *
 * group_ptr : A pointer to a buffer where ISI will write a string representing
 *             the "group" that is associated with the URI (jid) in user_ptr.
 *             This value is optional.
 *
 * name_ptr  : A pointer to a buffer where ISI will write a string representing
 *             a plain text "name" that is associated with the URI (jid) in
 *             user_ptr. This value is optional.
 *
 * subscription_ptr : A pointer to a buffer where ISI will write a string
 *             representing the state of the subscription.  The format of this
 *             string is defined by the underlying protocol. This value is
 *             mandatory.
 *
 * len      : The max length of the subscription_ptr buffer.
 *
 *
 * ISI_RETURN_OK                 : Roster information returned.
 * ISI_RETURN_DONE               : All Roster information returned has been
 *                                 returned.
 * ISI_RETURN_FAILED             : Roster information not found.
 * ISI_RETURN_INVALID_PRESENCE_ID: The presId parameter was invalid.
 * ISI_RETURN_NOT_INIT           : The ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Presence linked list.
 */
ISI_Return ISI_readContact(
    ISI_Id  presId,
    char   *user_ptr,
    char   *group_ptr,
    char   *name_ptr,
    char   *subscription_ptr,
    int     len)
{
    ISID_PresId   *pres_ptr;
    ISIP_Presence *p_ptr;
    vint           strLen;
    char          *str_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!user_ptr || !subscription_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the presence transaction via the presId */
    if (ISID_presGet(presId, &pres_ptr) != ISI_RETURN_OK) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_PRESENCE_ID);
    }

    p_ptr = &pres_ptr->isiMsg.msg.presence;

    OSAL_strncpy(user_ptr, p_ptr->from, ISI_ADDRESS_STRING_SZ);

    if (group_ptr) {
        OSAL_strncpy(group_ptr, p_ptr->roster.group, ISI_ADDRESS_STRING_SZ);
    }

    if (name_ptr) {
        OSAL_strncpy(name_ptr, p_ptr->roster.name, ISI_ADDRESS_STRING_SZ);
    }

    str_ptr = p_ptr->presence + pres_ptr->presOffset;
    strLen  = pres_ptr->presLen - pres_ptr->presOffset;
    OSAL_strncpy(subscription_ptr, str_ptr, len);
    if (strLen >= len) {
        /*
         * Then the output buffer was too short for the whole thing
         * update the position in the from buffer
         */
        len--;
        pres_ptr->presOffset += len;
        ISID_unlockPres();
        return (ISI_RETURN_OK);
    }

    /*
     * The whole "subscription state" has been
     * written so free up the resource
     */
    ISID_presDestroy(presId);
    ISID_unlockPres();
    return (ISI_RETURN_DONE);
}

/*
 * ======== ISI_subscribeToPresence() ========
 * This function is used to subscribe to presence of a remote entity.
 * Presence information is maintained at a presence server. Since the function
 * is used to subscribe for presence, ISI updates application every time
 * presence state of the remote entity changes. When presence information
 * of a remote entity is updated, ISI notifies application using ISI event
 * ISI_EVENT_PRES_RECEIVED event.  Application can read presence information
 * by calling function ISI_readPresence(). First argument to this function,
 * presId_ptr, is a pointer to memory location where ISI will write an
 * identification number for this presence transaction. ISI will notify
 * application of successful subscription or a failure using ISI events
 * ISI_EVENT_SUB_TO_PRES_SEND_OK or ISI_EVENT_SUB_TO_PRES_SEND_FAILED
 * argument to this function, serviceId, is the service identification number
 * returned by ISI when ISI_allocService() was called. Third argument to this
 * function, user_ptr, points to a null terminated string containing the
 * URI (jid) of the user to be removed from the list.
 *
 * presId_ptr : A pointer to memory location where ISI will write an
 *              identification number for this presence transaction.
 *              If this function returns a failure, this value will be invalid.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * user_ptr :  A null terminated string containing URI (jid) of the remote
 *             entity that we wish to subscribe to.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Request successfully sent to server.
 * ISI_RETURN_FAILED             : Could not send request to server.
 * ISI_RETURN_NOT_INIT           : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Presence linked list.
 */
ISI_Return ISI_subscribeToPresence(
    ISI_Id *presId_ptr,
    ISI_Id  serviceId,
    char   *user_ptr)
{
    ISID_ServiceId  *service_ptr;
    ISID_PresId     *pres_ptr;
    ISI_Id           presId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Check for required parameters */
    if (!presId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    if (!user_ptr || *user_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_OK) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    if ((pres_ptr = ISID_presCreate()) == NULL) {
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }

    pres_ptr->service_ptr = service_ptr;
    pres_ptr->reason = ISIP_PRES_REASON_SUB_TO_PRES;

    /* This will force the DB sub module to assign an id */
    presId = 0;
    if (ISID_presAdd(pres_ptr, &presId) == ISI_RETURN_FAILED) {
        /* Free the memory and return */
        ISI_free(pres_ptr, ISI_OBJECT_PRES_ID);
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISI Message and notify the protocol */
    ISIM_pres(pres_ptr, ISIP_PRES_REASON_SUB_TO_PRES, user_ptr);
    ISIR_appMsg(pres_ptr, &pres_ptr->isiMsg);
    *presId_ptr = presId;
    ISID_unlockPres();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_unsubscribeFromPresence() ========
 * This function is used to cancel subscription to presence of a presentity.
 * The function initiates a transaction for unsubscribing from presence
 * information of a presentity.  Application must have previously subscribed
 * to presence for which it is unsubscribing.
 *
 * presId_ptr : A pointer to memory location where ISI will write an
 *              identification number for this presence transaction.
 *              If this function returns a failure, this value will be invalid.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * user_ptr :  A null terminated string containing URI (jid) of the remote
 *             entity that we wish to unsubscribe to.
 *
 * ISI_RETURN_OK                  : Presence unsubscribe transaction started
 *                                  successfully.
 * ISI_RETURN_FAILED              : Could not start presence unsubscribe
 *                                  transaction.
 * ISI_RETURN_INVALID_PRESENCE_ID : Presence ID is invalid.
 * ISI_RETURN_NOT_INIT            : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the Presence linked list.
 *
 */
ISI_Return ISI_unsubscribeFromPresence(
    ISI_Id *presId_ptr,
    ISI_Id  serviceId,
    char   *user_ptr)
{
    ISID_ServiceId  *service_ptr;
    ISID_PresId     *pres_ptr;
    ISI_Id           presId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Check for required parameters */
    if (!presId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    if (!user_ptr || *user_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_OK) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    if ((pres_ptr = ISID_presCreate()) == NULL) {
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }

    pres_ptr->service_ptr = service_ptr;
    pres_ptr->reason = ISIP_PRES_REASON_UNSUB_TO_PRES;

    /* This will force the DB sub module to assign an id */
    presId = 0;
    if (ISID_presAdd(pres_ptr, &presId) == ISI_RETURN_FAILED) {
        /* Free the memory and return */
        ISI_free(pres_ptr, ISI_OBJECT_PRES_ID);
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISI Message and notify the protocol */
    ISIM_pres(pres_ptr, ISIP_PRES_REASON_UNSUB_TO_PRES, user_ptr);
    ISIR_appMsg(pres_ptr, &pres_ptr->isiMsg);
    *presId_ptr = presId;
    ISID_unlockPres();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_readSubscribeToPresenceRequest() ========
 * Application calls this function to read a subscription request
 * when the ISI event ISI_EVENT_SUB_TO_PRES_RECEIVED is received.
 * First argument to this function, presId, contains a unique
 * identification number representing the subscription transaction being
 * requested. Second argument to this function, user_ptr, is a pointer to
 * memory location where ISI will write the URI (jid) of the remote entity that
 * is requesting to receive subscription state information from this entity.
 *
 * presId    : A unique identification number representing the subscription
 *             request entry to retrieve.
 *
 * user_ptr  : A pointer to a buffer where ISI will write a string representing
 *             the remote entity's URI (or jid) of the subscription request.
 *
 *
 * ISI_RETURN_OK                 : Subscription information returned.
 * ISI_RETURN_FAILED             : Subscription information not found.
 * ISI_RETURN_INVALID_PRESENCE_ID: The presId parameter was invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Presence linked list.
 */
ISI_Return ISI_readSubscribeToPresenceRequest(
    ISI_Id  presId,
    char   *user_ptr)
{
    ISID_PresId  *pres_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!user_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the presence transaction via the presId */
    if (ISID_presGet(presId, &pres_ptr) != ISI_RETURN_OK) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_PRESENCE_ID);
    }

    OSAL_strncpy(user_ptr, pres_ptr->isiMsg.msg.presence.from,
            ISI_ADDRESS_STRING_SZ);

    ISID_presDestroy(presId);
    ISID_unlockPres();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_readSubscribeToPresenceResponse() ========
 * Application calls this function to read a result to a previous subscription
 * request that was made via "ISI_subscribeToPresence()". When the result is
 * received, the ISI will generate a ISI_EVENT_PRES_SUB_STATUS_RECEIVED
 * event to the application. Then the application can get the result via this
 * function. First argument to this function, presId, contains a unique
 * identification number representing the subscription result being requested.
 * Second argument to this function, user_ptr, is a pointer to memory location
 * where ISI will write the address (jid) of the remote entity that is
 * requesting to receive subscription state information.  Third argument,
 * isAllowed_ptr, is a pointer to data that will be populated with whether or
 * not the subscription request was successful.  If it is populated with '0',
 * then The subscription state was NOT allowed, otherwise the subscription is
 * allowed.
 *
 * Note, applications can receive a ISI_EVENT_PRES_SUB_STATUS_RECEIVED event
 * at any time and not just when the a subscription request is stimulated via
 * a call to ISI_sendSubscribeToPresence().  This event can also be received
 * when a remote entity refuses to continue to provide this entity presence
 * state. Applications can use ISI_readSubscriptionStatus() anytime a
 * ISI_EVENT_PRES_SUB_STATUS_RECEIVED event is received.
 *
 *
 * presId    : A unique identification number representing the subscription
 *             status entry to retrieve.
 *
 * user_ptr  : A pointer to a buffer where ISI will write a string representing
 *             the remote entity's URI (or jid) of the subscription status.
 *
 * isAllowed : A pointer to data where ISI will write a value representing
 *             the subscription status.  If '1' then the subscriptions status
 *             is considered to be "allowed".  Otherwise, the status is
 *             considered to be "not allowed".
 *
 * ISI_RETURN_OK                 : Subscription information returned.
 * ISI_RETURN_FAILED             : Subscription information not found.
 * ISI_RETURN_INVALID_PRESENCE_ID: The presId parameter was invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Presence linked list.
 */
ISI_Return ISI_readSubscriptionToPresenceResponse(
    ISI_Id  presId,
    char   *user_ptr,
    int    *isAllowed_ptr)
{
    ISID_PresId   *pres_ptr;
    ISIP_Presence *p_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Check that the required parameters are present */
    if (!user_ptr || !isAllowed_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the presence transaction via the presId */
    if (ISID_presGet(presId, &pres_ptr) != ISI_RETURN_OK) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_PRESENCE_ID);
    }

    p_ptr = &pres_ptr->isiMsg.msg.presence;

    OSAL_strncpy(user_ptr, p_ptr->from, ISI_ADDRESS_STRING_SZ);

    if (p_ptr->reason == ISIP_PRES_REASON_SUB_ALLOWED) {
        *isAllowed_ptr = 1;
    }
    else {
        *isAllowed_ptr = 0;
    }

    ISID_presDestroy(presId);
    ISID_unlockPres();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_allowSubscriptionToPresence() ========
 * This function is used to allow a subscription that was previously
 * requested by the remote party when a ISI_EVENT_SUB_TO_PRES_RECEIVED
 * event was received. The function initiates a transaction for reporting
 * the allowance of a subscription to presence.
 *
 * presId_ptr : A pointer to memory location where ISI will write an
 *              identification number for this presence transaction.
 *              If this function returns a failure, this value will be invalid.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * user_ptr :  A null terminated string containing URI (jid) of the remote
 *             entity that we wish to allow.
 *
 * Return Values:
 * ISI_RETURN_OK                  : Transaction for allowing a subscription
 *                                  was sent.
 * ISI_RETURN_FAILED              : Transaction for allowing a subscription
 *                                  could not be sent.
 * ISI_RETURN_NOT_INIT            : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the Presence linked list.
 * ISI_RETURN_INVALID_SERVICE_ID  : The 'serviceId was invalid.
 *
 */
ISI_Return ISI_allowSubscriptionToPresence(
    ISI_Id *presId_ptr,
    ISI_Id  serviceId,
    char   *user_ptr)
{
    ISID_ServiceId  *service_ptr;
    ISID_PresId     *pres_ptr;
    ISI_Id           presId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Check for required parameters */
    if (!presId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    if (!user_ptr || *user_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_OK) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    if ((pres_ptr = ISID_presCreate()) == NULL) {
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }

    pres_ptr->service_ptr = service_ptr;
    pres_ptr->reason = ISIP_PRES_REASON_SUB_ALLOWED;

    /* This will force the DB sub module to assign an id */
    presId = 0;
    if (ISID_presAdd(pres_ptr, &presId) == ISI_RETURN_FAILED) {
        /* Free the memory and return */
        ISI_free(pres_ptr, ISI_OBJECT_PRES_ID);
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISI Message and notify the protocol */
    ISIM_pres(pres_ptr, ISIP_PRES_REASON_SUB_ALLOWED, user_ptr);
    ISIR_appMsg(pres_ptr, &pres_ptr->isiMsg);
    *presId_ptr = presId;
    ISID_unlockPres();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_denySubscriptionToPresence() ========
 * This function is used to deny a subscription that was previously
 * requested by the remote party when a ISI_EVENT_SUB_TO_PRES_RECEIVED
 * event was received. The function initiates a transaction for reporting
 * the denial of a subscription to presence.
 *
 * presId_ptr : A pointer to memory location where ISI will write an
 *              identification number for this presence transaction.
 *              If this function returns a failure, this value will be invalid.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * user_ptr :  A null terminated string containing URI (jid) of the remote
 *             entity that we wish to deny.
 *
 * Return Values:
 * ISI_RETURN_OK                  : Transaction for denying a subscription
 *                                  was sent.
 * ISI_RETURN_FAILED              : Transaction for denying a subscription
 *                                  could not be sent.
 * ISI_RETURN_NOT_INIT            : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the Presence linked list.
 * ISI_RETURN_INVALID_SERVICE_ID  : The 'serviceId was invalid.
 *
 */
ISI_Return ISI_denySubscriptionToPresence(
    ISI_Id *presId_ptr,
    ISI_Id  serviceId,
    char   *user_ptr)
{
    ISID_ServiceId  *service_ptr;
    ISID_PresId     *pres_ptr;
    ISI_Id           presId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Check for required parameters */
    if (!presId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    if (!user_ptr || *user_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_OK) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    if ((pres_ptr = ISID_presCreate()) == NULL) {
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }

    pres_ptr->service_ptr = service_ptr;
    pres_ptr->reason = ISIP_PRES_REASON_SUB_REFUSED;

    /* This will force the DB sub module to assign an id */
    presId = 0;
    if (ISID_presAdd(pres_ptr, &presId) == ISI_RETURN_FAILED) {
        /* Free the memory and return */
        ISI_free(pres_ptr, ISI_OBJECT_PRES_ID);
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISI Message and notify the protocol */
    ISIM_pres(pres_ptr, ISIP_PRES_REASON_SUB_REFUSED, user_ptr);
    ISIR_appMsg(pres_ptr, &pres_ptr->isiMsg);
    *presId_ptr = presId;
    ISID_unlockPres();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_sendPresence() ========
 * Applications call this function to update presence state information of this
 * entity.
 *
 * presId_ptr : a pointer to memory location where ISI will write an
 *              identification number for this presence transaction.
 *              If this function returns a failure, this value will be invalid.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called
 *
 * to_ptr :    A null terminated string containing URI (jid) of the entity
 *             to send presence to. If NULL, then the presence state will be
 *             sent to the server associated with the service.
 *
 * presence_ptr : A null terminated string containing the protocol specific
 *                presence string.  This string represents the entities actual
 *                state of presence. Typically the format of this string is XML.
 *
 * Return Values:
 * ISI_RETURN_OK            : The Presence information was successfully sent.
 * ISI_RETURN_FAILED        : Function Failed.  The Presence information
 *                            could not be sent.
 * ISI_RETURN_NOT_INIT      : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR   : Could not lock the Presence linked list.
 */
ISI_Return ISI_sendPresence(
    ISI_Id *presId_ptr,
    ISI_Id  serviceId,
    char   *to_ptr,
    char   *presence_ptr)
{
    ISID_ServiceId  *service_ptr;
    ISIP_Presence   *p_ptr;
    ISID_PresId     *pres_ptr;
    ISI_Id           presId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Check for required parameters */
    if (!presId_ptr || !presence_ptr || *presence_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_OK) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    if ((pres_ptr = ISID_presCreate()) == NULL) {
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }

    p_ptr = &pres_ptr->isiMsg.msg.presence;
    pres_ptr->service_ptr = service_ptr;
    pres_ptr->reason = ISIP_PRES_REASON_PRESENCE;
    OSAL_strncpy(p_ptr->presence, presence_ptr, ISI_PRESENCE_STRING_SZ);

    /* This will force the DB sub module to assign an id */
    presId = 0;
    if (ISID_presAdd(pres_ptr, &presId) == ISI_RETURN_FAILED) {
        /* Free the memory and return */
        ISI_free(pres_ptr, ISI_OBJECT_PRES_ID);
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISI Message and notify the protocol */
    ISIM_pres(pres_ptr, ISIP_PRES_REASON_PRESENCE, to_ptr);
    ISIR_appMsg(pres_ptr, &pres_ptr->isiMsg);
    *presId_ptr = presId;
    ISID_unlockPres();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_readPresence() ========
 * Applications call this function to read presence information of an entity
 * when a ISI event ISI_EVENT_PRES_RECEIVED is received by the application.
 *
 * characters that ISI can write from_ptr are ISI_ADDRESS_STRING_SZ.
 *
 * If the 'len' is smaller than the length of the presence information, ISI
 * will place partial presence information in this buffer.  Application must
 * call ISI_readPresence() repeatedly to receive complete presence information.
 * When ISI has delivered the complete presence state information to ISI, it
 * will return ISI_RETURN_DONE.
 *
 * presId    : A unique identification number representing the presence
 *             transaction entry to retrieve.
 *
 * chatId_ptr : a pointer to memory location where ISI will write the call
 *              identification number for the call whose context this
 *              presence was received. If the presence transaction is outside
 *              the context of a call then ISI will write a '0' to
 *              this memory location.
 *
 * from_ptr :  A pointer to a buffer where ISI will write a string representing
 *             the entity who's presence state is being signalled. the size of
 *             the buffer pointer to by the value MUST be ISI_ADDRESS_STRING_SZ
 *             bytes long.
 *
 * presence_ptr : A pointer to memory location where ISI will write presence
 *                information as a null terminated string.
 *
 * len : The size of the buffer pointed to by presence_ptr.  Note, if the length
 *       of this buffer is too small for ISI to write the entire presence string
 *       then ISI will write a portion of the presence string are return
 *       ISI_RETURN_OK. In this case, the application MUST call this routine
 *       repeatedly until ISI_RETURN_DONE is returned to ensure the entire
 *       presence string was read.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Presence information returned. But there is
 *                                 still more to be read.
 * ISI_RETURN_DONE               : All Presence information has been returned.
 * ISI_RETURN_FAILED             : Presence information not found.
 * ISI_RETURN_INVALID_PRESENCE_ID: The presId parameter was invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Presence linked list.
 */
ISI_Return ISI_readPresence(
    ISI_Id  presId,
    ISI_Id *callId_ptr,
    char   *from_ptr,
    char   *presence_ptr,
    int     len)
{
    ISID_PresId   *pres_ptr;
    ISIP_Presence *p_ptr;
    vint           strLen;
    char          *str_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!presence_ptr || !from_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the presence transaction via the presId */
    if (ISID_presGet(presId, &pres_ptr) != ISI_RETURN_OK) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_PRESENCE_ID);
    }

    p_ptr = &pres_ptr->isiMsg.msg.presence;

    /* Set the from */
    OSAL_strncpy(from_ptr, p_ptr->from, ISI_ADDRESS_STRING_SZ);

    str_ptr = p_ptr->presence + pres_ptr->presOffset;
    strLen  = pres_ptr->presLen - pres_ptr->presOffset;
    OSAL_strncpy(presence_ptr, str_ptr, len);
    *callId_ptr = pres_ptr->chatId;
    if (strLen >= len) {
        /*
         * Then the output buffer was too short for the whole thing
         * update the position in the from buffer
         */
        len--;
        pres_ptr->presOffset += len;
        ISID_unlockPres();
        return (ISI_RETURN_OK);
    }

    /* The whole presence action has been written so free up the resource */
    ISID_presDestroy(presId);
    ISID_unlockPres();
    return (ISI_RETURN_DONE);
}


/*
 * ======== ISI_sendCapabilities() ========
 * Applications call this function to update capabilities information of this
 * entity.
 *
 * capsId_ptr : a pointer to memory location where ISI will write an
 *              identification number for this capabilities transaction.
 *              If this function returns a failure, this value will be invalid.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called
 *
 * to_ptr :    A null terminated string containing URI (jid) of the entity
 *             to send capabilities to. If NULL, then the capabilities info will
 *             be sent to the server associated with the service.
 *
 * capabilities_ptr : A null terminated string containing the protocol specific
 *             capabilities string.  This string represents the entity's actual
 *             state of capabilities. Typically the format of the string is XML.
 *
 * priority : This flag is set to 1 to indicate the request should be treated
 *            with priority.  It is used to signify this request should be
 *            processed before any next presence message, but it is not
 *            guaranteed.
 *
 * Return Values:
 * ISI_RETURN_OK            : The capabilities info was successfully sent.
 * ISI_RETURN_FAILED        : Function Failed.  The capabilities information
 *                            could not be sent.
 * ISI_RETURN_INVALID_SERVICE_ID: Service ID is not valid.
 * ISI_RETURN_SERVICE_NOT_ACTIVE: Service is not logged in.
 * ISI_RETURN_NOT_INIT      : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR   : Could not lock the capabilities linked list.
 */
ISI_Return ISI_sendCapabilities(
    ISI_Id *capsId_ptr,
    ISI_Id  serviceId,
    char   *to_ptr,
    char   *capabilities_ptr,
    int     priority)
{
    int ret;
    ISID_ServiceId  *service_ptr;
    ISIP_Presence   *p_ptr;
    ISID_PresId     *caps_ptr;
    ISI_Id           capsId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Check for required parameters */
    if (!capsId_ptr || !capabilities_ptr || *capabilities_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    ret = ISID_serviceGet(serviceId, &service_ptr);
    if (ret == ISI_RETURN_FAILED) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }
    else if (ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
        ISID_unlockPres();
        return (ISI_RETURN_SERVICE_NOT_ACTIVE);
    }

    if ((caps_ptr = ISID_presCreate()) == NULL) {
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }

    p_ptr = &caps_ptr->isiMsg.msg.presence;
    caps_ptr->service_ptr = service_ptr;
    caps_ptr->reason = ISIP_PRES_REASON_CAPABILITIES_REQUEST;
    OSAL_strncpy(p_ptr->presence, capabilities_ptr, ISI_PRESENCE_STRING_SZ);

    /* This will force the DB sub module to assign an id */
    capsId = 0;
    if (priority == 1) {
        if (ISID_presAddFirst(caps_ptr, &capsId) == ISI_RETURN_FAILED) {
            /* Free the memory and return */
            ISI_free(caps_ptr, ISI_OBJECT_PRES_ID);
            ISID_unlockPres();
            return (ISI_RETURN_FAILED);
        }
    }
    else {
        if (ISID_presAdd(caps_ptr, &capsId) == ISI_RETURN_FAILED) {
            /* Free the memory and return */
            ISI_free(caps_ptr, ISI_OBJECT_PRES_ID);
            ISID_unlockPres();
            return (ISI_RETURN_FAILED);
        }
    }

    /* Populate an ISI Message and notify the protocol */
    ISIM_pres(caps_ptr, ISIP_PRES_REASON_CAPABILITIES_REQUEST, to_ptr);
    ISIR_appMsg(caps_ptr, &caps_ptr->isiMsg);
    *capsId_ptr = capsId;
    ISID_unlockPres();


    return (ISI_RETURN_OK);
}


/*
 * ======== ISI_readCapabilities() ========
 * Applications call this function to read capability information of an entity
 * when a ISI event ISI_EVENT_CAPS_RECEIVED is received by the application.
 *
 * characters that ISI can write from_ptr are ISI_ADDRESS_STRING_SZ.
 *
 * If the 'len' is smaller than the length of the capability information, ISI
 * will place partial capability information in this buffer.  Application must
 * call ISI_readCapability() repeatedly to receive complete set of information.
 * When ISI has delivered the complete capability state information to ISI, it
 * will return ISI_RETURN_DONE.
 *
 * capsId    : A unique identification number representing the capability
 *             transaction entry to retrieve.
 * capsId_ptr : a pointer to memory location where ISI will write the
 *              identification number for the chat room in whose context this
 *              presence was received. If the presence transaction is outside
 *              the context of a group chat room then ISI will write a '0' to
 *              this memory location.
 *
 * from_ptr :  A pointer to a buffer where ISI will write a string representing
 *             the entity who's presence state is being signalled. the size of
 *             the buffer pointer to by the value MUST be ISI_ADDRESS_STRING_SZ
 *             bytes long.
 *
 * capabilities_ptr : A pointer to memory location where ISI will write
 *                capabilities information as a null terminated string.
 *
 * len : The size of the buffer pointed to by capabilities_ptr.  Note, if length
 *       of this buffer is too small for ISI to write the entire presence string
 *       then ISI will write a portion of the presence string are return
 *       ISI_RETURN_OK. In this case, the application MUST call this routine
 *       repeatedly until ISI_RETURN_DONE is returned to ensure the entire
 *       presence string was read.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Capability information returned. But there is
 *                                 still more to be read.
 * ISI_RETURN_DONE               : All Capability information has been returned.
 * ISI_RETURN_FAILED             : Capability information not found.
 * ISI_RETURN_INVALID_PRESENCE_ID: The capsId parameter was invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Presence linked list,
 *                                  which is used for capability processing.
 */
ISI_Return ISI_readCapabilities(
    ISI_Id  capsId,
    ISI_Id *capsId_ptr,
    char   *from_ptr,
    char   *capabilities_ptr,
    int     len)
{
    ISID_PresId   *pres_ptr;
    ISIP_Presence *p_ptr;
    vint           strLen;
    char          *str_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!capabilities_ptr || !from_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the presence transaction via the presId */
    if (ISID_presGet(capsId, &pres_ptr) != ISI_RETURN_OK) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_PRESENCE_ID);
    }

    p_ptr = &pres_ptr->isiMsg.msg.presence;

    /* Set the from */
    OSAL_strncpy(from_ptr, p_ptr->from, ISI_ADDRESS_STRING_SZ);

    str_ptr = p_ptr->presence + pres_ptr->presOffset;
    strLen  = pres_ptr->presLen - pres_ptr->presOffset;
    OSAL_strncpy(capabilities_ptr, str_ptr, len);
    *capsId_ptr = pres_ptr->chatId;
    if (strLen >= len) {
        /*
         * Then the output buffer was too short for the whole thing
         * update the position in the from buffer
         */
        len--;
        pres_ptr->presOffset += len;
        ISID_unlockPres();
        return (ISI_RETURN_OK);
    }

    /* The whole presence action has been written so free up the resource */
    ISID_presDestroy(capsId);
    ISID_unlockPres();
    return (ISI_RETURN_DONE);
}

/*
 * ======== _ISI_processProtocolEvent() ========
 * This function handle all protocol commands that COME FROM the underlying protocol.
 * It will dispatch to the FSM entry point for each protocol type (call, media, ...etc)
 *
 * msg_ptr : point to the ISIP protocol message to dispatch
 *
 * Return Values:
 * ISI_RETURN_OK       : protocol event processed.
 * ISI_RETURN_FAILED   : unknown protocol event.
 */
ISI_Return _ISI_processProtocolEvent(
    ISIP_Message     *msg_ptr)
{
    switch(msg_ptr->code) {
        case ISIP_CODE_MEDIA:
            ISIM_protoMsg(msg_ptr);
            break;
        case ISIP_CODE_SERVICE:
            ISIS_protoMsg(msg_ptr->id, msg_ptr);
            break;
        case ISIP_CODE_CALL:
            ISIC_protoMsg(msg_ptr->id, msg_ptr);
            break;
        case ISIP_CODE_MESSAGE:
            ISIT_protoMsg(msg_ptr->id, msg_ptr);
            break;
        case ISIP_CODE_TEL_EVENT:
            ISIE_protoMsg(msg_ptr->id, msg_ptr);
            break;
        case ISIP_CODE_PRESENCE:
            ISIR_protoMsg(msg_ptr->id, msg_ptr);
            break;
        case ISIP_CODE_CHAT:
            ISIG_protoMsg(msg_ptr->id, msg_ptr);
            break;
        case ISIP_CODE_SYSTEM:
            ISIY_protoMsg(msg_ptr);
            break;
        case ISIP_CODE_FILE:
            ISIFT_protoMsg(msg_ptr->id, msg_ptr);
            break;
        case ISIP_CODE_USSD:
            ISIT_protoUssd(msg_ptr->id, msg_ptr);
            break;
        case ISIP_CODE_LAST:
            break;
        case ISIP_CODE_INVALID:
        default:
            return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_getEvent() ========
 * Applications use this function to receive events from ISI.
 * Note that this is a blocking call.  When the application calls this API it
 * will wait until a event for the application becomes available. If this
 * function returns ISI_RETURN_OK, then the parameters will contain the
 * details of the event.
 *
 * serviceId_ptr : A pointer to a ISI_Id representing the service that this
 *                event is for.
 *
 * id_ptr : A pointer to a ISI_Id.  If ISI_RETURN_OK is returned then this
 *          this value will be populated with the CallId, ServiceId, presenceId,
 *          messageId, etc.
 *
 * idType_ptr : A pointer to a ISI_IdType that describes the type of
 *              ID type populated in id_ptr.
 *
 * event_ptr : A pointer to a ISI_Event enumeration that describes the
 *             event that has just been reported from the ISI.
 *
 * eventDesc_ptr : A NULL terminated string that includes any additional
 *                 information (human readible info) about the event when
 *                 additional information is available.  If no other
 *                 info is available then a STRING representing the event is
 *                 return.  This can be used for debugging purposes.
 *                 The buffer pointed to by eventDesc_ptr must be at least
 *                 ISI_EVENT_DESC_STRING_SZ bytes long.
 *
 * Return Values:
 * ISI_RETURN_OK       : An Event was returned.
 * ISI_RETURN_FAILED   : There is currently no valid event. This means that
 *                       there was a problem with the underlying select
 *                       statement or with reading an underlying IPC mechanism.
 * ISI_RETURN_TIMEOUT  : The function timed out. There's no event to process.
 * ISI_RETURN_NOT_INIT : ISI not initialized.
 */
ISI_Return ISI_getEvent(
    ISI_Id     *serviceId_ptr,
    ISI_Id     *id_ptr,
    ISI_IdType *idType_ptr,
    ISI_Event  *event_ptr,
    char       *eventDesc_ptr,
    int         timeout)
{
    ISI_EventMessage *evt_ptr;
    ISIP_Message     *msg_ptr;
    ISI_Return        ret;
    vint              isApplication;
    vint              lenBuffer;

    /*
     * XXX Comment out this and use heap instead.
     * Here's the buffer.  Allocate in .bss.
     * Make it the biggest size possible which is the
     * protocol message size
    char buffer[sizeof(ISIP_Message) + 1];
     */

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    while (1) {
        lenBuffer = sizeof(ISIP_Message);
        ret = ISIQ_getEvent(_buf_ptr, &lenBuffer, &isApplication, timeout);

        /* OSAL_logMsg("ISIQ_getEvent returned: isApp=%d\n", isApplication); */

        if (ret == ISI_RETURN_OK) {

            /*
             * If ISIQ_getEvent() returns OK then the lenBuffer param
             * will contain the length of the buffer just read
             */
            if (isApplication) {
                /*
                 * Then the data came from a pipe used to queue
                 * events for the application
                 */
                evt_ptr = (ISI_EventMessage*)_buf_ptr;

                if (ISI_ID_TYPE_INVALID != evt_ptr->idType) {
                    /* Then give the event to the application */
                    *serviceId_ptr = evt_ptr->serviceId;
                    *id_ptr        = evt_ptr->id;
                    *idType_ptr    = evt_ptr->idType;
                    *event_ptr     = evt_ptr->event;
                    if (NULL != eventDesc_ptr) {
                        if (0 != evt_ptr->eventDesc[0]) {
                            OSAL_strncpy(eventDesc_ptr, evt_ptr->eventDesc,
                                    ISI_EVENT_DESC_STRING_SZ);
                        }
                        else {
                            *eventDesc_ptr = 0;
                        }
                    }
                    return (ISI_RETURN_OK);
                }
                else {
                    if (!ISID_getState()) {
                        /*
                         * If we were woken up and the ISI state is '0' that
                         * means we are shutting down.  Tell the calling app
                         * that we are not initialized.
                         */
                        return (ISI_RETURN_NOT_INIT);
                    }
                }
            }
            else {
                /*
                 * The data is from the pipe used for the protocol queue
                 */
                msg_ptr = (ISIP_Message*)_buf_ptr;
                
                _ISI_processProtocolEvent(msg_ptr);
            }
        }
        else if (ret == ISI_RETURN_TIMEOUT) {
            /*
             * Then the underlying select() statement timed out.  There is no
             * data to process return 'failure' to the application
             */
            return (ISI_RETURN_TIMEOUT);
        }
        else {
            /*
             * There was an error with the underlying select call.  There
             * is no data for the application to process so return a failure.
             */
            return (ISI_RETURN_FAILED);
        }
    }
}

/*
 * ======== ISI_setChatNickname() ========
 * This function is used to set a nickname to use for chat sessions for a service.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * name_ptr : A mandatory null terminated string containing the nickname
 *            to use for chat sessions.
 *
 * Return Values:
 * ISI_RETURN_OK                   : Successful.
 * ISI_RETURN_FAILED               : invalid name_ptr parameter.
 * ISI_RETURN_INVALID_SERVICE_ID   : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the services linked list.
 */
ISI_Return ISI_setChatNickname(
    ISI_Id  serviceId,
    char   *name_ptr)
{
    ISID_ServiceId *service_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!name_ptr || *name_ptr == 0) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        OSAL_strncpy(service_ptr->szNickname, name_ptr, ISI_ADDRESS_STRING_SZ);
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_initiateGroupChat() ========
 * This function is used to initiate a group chat session using the service
 * specified by serviceId. ISI will generate ISI events indicating the state of
 * the group chat session, for example: ISI_EVENT_CHAT_ACCEPTED,
 * ISI_EVENT_CHAT_DISCONNECTED, ISI_EVENT_CHAT_FAILED.
 *
 * chatId_ptr : A pointer to a unique identification number.  ISI will populate
 *              this value with a unique ID representing the this specific
 *              group chat session.  This value is used in subsequent ISI
 *              API's related to group chat sessions.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * subject_ptr : An optional parameter that can contain a null terminated string
 *               representing the 'subject' matter of the group chat
 *               conversation.
 *
 * password_ptr :  An optional parameter that can contain a null terminated
 *                string representing the password required to enter the group
 *                chat session.
 *
 * sessionType : Indicate if this chat is use TLS.
 *
 * Return Values:
 * ISI_RETURN_OK                   : Group Chat initiation was successful.
 * ISI_RETURN_FAILED               : Failed to initiate a group chat room.
 * ISI_RETURN_INVALID_SERVICE_ID   : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the group chat linked list.
 */
ISI_Return ISI_initiateGroupChat(
    ISI_Id             *chatId_ptr,
    ISI_Id              serviceId,
    char               *roomName_ptr,
    char               *subject_ptr,
    char               *password_ptr,
    ISI_SessionType     sessionType)
{
    ISID_ServiceId *service_ptr;
    ISID_GChatId   *chat_ptr;
    ISI_Id          chatId;
    ISIP_Message   *msg_ptr;
    vint            passwordRequired = 0;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Validate the mandatory params */
    if (!chatId_ptr || !roomName_ptr || 0 == *roomName_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of group chat rooms */
    if (ISI_RETURN_OK != ISID_lockGChats()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISI_RETURN_OK == ISID_serviceGet(serviceId, &service_ptr)) {
        /*
         * Then we know the id is valid and active.
         */
        if (password_ptr && 0 != *password_ptr) {
            passwordRequired = 1;
        }
        if (NULL == (chat_ptr = ISID_gchatCreate(service_ptr, roomName_ptr,
                service_ptr->szNickname, service_ptr->szNickname, subject_ptr, NULL,
                passwordRequired))) {
            ISID_unlockGChats();
            return (ISI_RETURN_FAILED);
        }
        else {
            /* Add it to the database.  Init the chatId pointer to zero
             * This indicates to the DB that we wish for the DB to determine
             * the Chat Id.
             */
            chatId = 0;
            if (ISI_RETURN_OK != ISID_gchatAdd(chat_ptr, &chatId)) {
                /* Free the memory and return */
                ISI_free(chat_ptr, ISI_OBJECT_GCHAT_ID);
                ISID_unlockGChats();
                return (ISI_RETURN_FAILED);
            }
            /* Now notify the protocol */
            if (NULL == (msg_ptr = ISIM_initiateChat(chat_ptr,
                    ISIP_GROUP_CHAT_REASON_INITIATE, NULL, password_ptr,
                    sessionType))) {
                ISID_gchatDestroy(chatId);
                ISID_unlockGChats();
                return (ISI_RETURN_FAILED);
            }
            if (ISI_RETURN_OK != ISIG_appMsg(chat_ptr, msg_ptr)) {
                ISID_gchatDestroy(chatId);
                ISID_unlockGChats();
                return (ISI_RETURN_FAILED);
            }
            *chatId_ptr = chatId;
            ISID_unlockGChats();
            return (ISI_RETURN_OK);
        }
    }
    ISID_unlockGChats();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_initiateChat() ========
 * This function is used to initiate a 1to1 chat session using the service
 * specified by serviceId. ISI will generate ISI events indicating the state of
 * the  chat session, for example: ISI_EVENT_CHAT_ACCEPTED,
 * ISI_EVENT_CHAT_DISCONNECTED, ISI_EVENT_CHAT_FAILED.
 *
 * chatId_ptr : A pointer to a unique identification number.  ISI will populate
 *              this value with a unique ID representing the this specific
 *              chat session.  This value is used in subsequent ISI
 *              API's related to group chat sessions.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * participant_ptr : A mandoatory NULL terminated string containing the address
 *                   off the remote party to initiate the chat with.
 *
 * subject_ptr : An optional parameter that can contain a null terminated string
 *               representing the 'subject' matter of the group chat
 *               conversation.
 *
 * message_ptr : An optional parameter containing a NULL terminated string
 *               withthe first instant message to include with the chat request.
 *               Note that the reports and reportId_ptr parameters are only
 *               realivant if this parameter is NOT NULL.
 *
 * reports : An optional parameter containing a bitmap representing the
 *           'delivery' and 'display' reports to request for the message
 *           provided in message_ptr.
 *
 * reportId_ptr :  An optional parameter containing a NUL terminated string with a
 *                'Report ID'; a unique identifier used to match 'delivery' & 'display'
 *                 reports with the message sent in message_ptr.
 *
 * sessionType : Indicate if this chat is use TLS.
 *
 * Return Values:
 * ISI_RETURN_OK                   : A request to estabpish a chat session was successful sent.
 * ISI_RETURN_FAILED               : Request failed.
 * ISI_RETURN_INVALID_SERVICE_ID   : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the chat linked list.
 */
ISI_Return ISI_initiateChat(
    ISI_Id            *chatId_ptr,
    ISI_Id             serviceId,
    char              *participant_ptr,
    char              *subject_ptr,
    char              *message_ptr,
    ISI_MessageReport  reports,
    char              *reportId_ptr,
    ISI_SessionType    sessionType)
{
    ISID_ServiceId *service_ptr;
    ISID_GChatId   *chat_ptr;
    ISI_Id          chatId;
    ISIP_Message   *msg_ptr;
    ISIP_Text      *t_ptr;
    vint            len;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Validate the mandatory params */
    if (!chatId_ptr ||
            !participant_ptr || 0 == *participant_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of group chat rooms */
    if (ISI_RETURN_OK != ISID_lockGChats()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISI_RETURN_OK == ISID_serviceGet(serviceId, &service_ptr)) {
        if (NULL == (chat_ptr = ISID_gchatCreate(service_ptr, participant_ptr,
                service_ptr->szUri, service_ptr->szUri, subject_ptr, participant_ptr, 0))) {
            ISID_unlockGChats();
            return (ISI_RETURN_FAILED);
        }
        else {
            /* Add it to the database.  Init the chatId pointer to zero
             * This indicates to the DB that we wish for the DB to determine
             * the Chat Id.
             */
            chatId = 0;
            if (ISI_RETURN_OK != ISID_gchatAdd(chat_ptr, &chatId)) {
                /* Free the memory and return */
                ISI_free(chat_ptr, ISI_OBJECT_GCHAT_ID);
                ISID_unlockGChats();
                return (ISI_RETURN_FAILED);
            }
            /* Now notify the protocol */
            if (NULL == (msg_ptr = ISIM_initiateChat(chat_ptr,
                    ISIP_CHAT_REASON_INITIATE,
                    participant_ptr, NULL, sessionType))) {
                ISID_gchatDestroy(chatId);
                ISID_unlockGChats();
                return (ISI_RETURN_FAILED);
            }


            if (message_ptr && 0 != *message_ptr) {
                t_ptr = &msg_ptr->msg.groupchat.text;

                /* Copy the message */
               len = OSAL_strlen(message_ptr);
               if (len >= (vint)sizeof(t_ptr->message)) {
                   ISID_gchatDestroy(chatId);
                   ISID_unlockGChats();
                   return (ISI_RETURN_FAILED);
               }
               OSAL_strncpy(t_ptr->message, message_ptr, sizeof(t_ptr->message));

               /* Copy the reports that we are requesting for this text message */
               t_ptr->report = reports;
               if (reports != ISI_MSG_RPT_NONE) {
                   /* Then we have an id value, so let's populate */
                   OSAL_strncpy(t_ptr->messageId, reportId_ptr, ISI_ID_STRING_SZ);
               }

               /* Populate the rest */
               t_ptr->reason = ISIP_TEXT_REASON_NEW;
               t_ptr->chatId = chatId;
               OSAL_strncpy(t_ptr->subject, chat_ptr->szSubject, ISI_SUBJECT_STRING_SZ);
               OSAL_strncpy(t_ptr->to, chat_ptr->szRemoteAddress, ISI_ADDRESS_STRING_SZ);
               OSAL_strncpy(t_ptr->from, chat_ptr->szLocalIdentity, ISI_ADDRESS_STRING_SZ);
            }
            if (ISI_RETURN_OK != ISIG_appMsg(chat_ptr, msg_ptr)) {
                ISID_gchatDestroy(chatId);
                ISID_unlockGChats();
                return (ISI_RETURN_FAILED);
            }
            *chatId_ptr = chatId;
            ISID_unlockGChats();
            return (ISI_RETURN_OK);
        }
    }
    ISID_unlockGChats();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_acceptChat() ========
 * This function is used to accept an invitation to a chat.
 * Invitations to chats are signalled to applications via the
 * ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE ISI event.
 *  Applications can call this routine to accept the invitation.
 *
 * chatId :    The ID of the chat session.  This is the value from the
 *             ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE ISI event.
 *
 * Return Values:
 * ISI_RETURN_OK                   : The invitation was successfully accepted.
 * ISI_RETURN_FAILED               : Failed to accept the invitation.
 * ISI_RETURN_INVALID_CHAT_ID      : The chat ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the chat linked list.
 */
ISI_Return ISI_acceptChat(
    ISI_Id   chatId)
{
    ISID_GChatId *chat_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISI_RETURN_OK != ISID_lockGChats()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if ( ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
        /* Then there is no active call */
        ISID_unlockGChats();
        return (ISI_RETURN_INVALID_CHAT_ID);
    }

    /* Now notify the protocol */
    if (NULL == (msg_ptr = ISIM_acceptChat(chat_ptr, NULL))) {
        ISID_unlockGChats();
        return (ISI_RETURN_FAILED);
    }
    if (ISI_RETURN_OK != ISIG_appMsg(chat_ptr, msg_ptr)) {
        ISID_unlockGChats();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockGChats();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_rejectChat() ========
 * This function is used to reject an invitation to a chat session.
 * Invitations to chat sessions are signalled to applications via the
 * ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE ISI event.
 * Applications can call this routine to  reject the invitation to the chat.
 *
 * chatId :     The ID of the chat session.  This is the value from the
 *              ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE ISI event.
 *
 * reason_ptr : An optional parameter that can contain a null terminated
 *              string describing the reason why the invitation was rejected. *
 *
 * Return Values:
 * ISI_RETURN_OK                   : Rejection successful.
 * ISI_RETURN_FAILED               : Failed to reject the invitation.
 * ISI_RETURN_INVALID_CHAT_ID      : The chat ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the chat linked list.
 */

ISI_Return ISI_rejectChat(
    ISI_Id   chatId,
    char    *reason_ptr)
{
    ISID_GChatId *chat_ptr;
   ISIP_Message *msg_ptr;

   if (!ISID_getState()) {
       return (ISI_RETURN_NOT_INIT);
   }

   /* Lock the database of group chats */
   if (ISI_RETURN_OK != ISID_lockGChats()) {
       return (ISI_RETURN_MUTEX_ERROR);
   }

   if (ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
       /* Then there is no active call */
       ISID_unlockGChats();
       return (ISI_RETURN_INVALID_CHAT_ID);
   }

   /* Now notify the protocol */
   if (NULL == (msg_ptr = ISIM_rejectChat(chat_ptr, reason_ptr))) {
       ISID_unlockGChats();
       return (ISI_RETURN_FAILED);
   }
   if (ISI_RETURN_OK != ISIG_appMsg(chat_ptr, msg_ptr)) {
       ISID_unlockGChats();
       return (ISI_RETURN_FAILED);
   }
   ISID_unlockGChats();
   return (ISI_RETURN_OK);
}

/*
 * ======== ISI_acknowledgeChat() ========
 * This function is used to acknowledge that an invitation to a chat
 * has been received and the user is currently being alerted of this request.
 * Invitations to chats are signalled to applications via the
 * ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE ISI event.
 *
 * chatId :    The ID of the chat session.  This is the value from the
 *             ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE ISI event.
 *
 * Return Values:
 * ISI_RETURN_OK                   : The invitation was successfully accepted.
 * ISI_RETURN_FAILED               : Failed to accept the invitation.
 * ISI_RETURN_INVALID_CHAT_ID      : The chat ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the chat linked list.
 */
ISI_Return ISI_acknowledgeChat(
    ISI_Id   chatId)
{
    ISID_GChatId *chat_ptr;
   ISIP_Message *msg_ptr;

   if (!ISID_getState()) {
       return (ISI_RETURN_NOT_INIT);
   }

   /* Lock the database of calls */
   if (ISI_RETURN_OK != ISID_lockGChats()) {
       return (ISI_RETURN_MUTEX_ERROR);
   }
   if ( ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
       /* Then there is no active call */
       ISID_unlockGChats();
       return (ISI_RETURN_INVALID_CHAT_ID);
   }

   /* Now notify the protocol */
   if (NULL == (msg_ptr = ISIM_ackChat(chat_ptr, NULL))) {
       ISID_unlockGChats();
       return (ISI_RETURN_FAILED);
   }
   if (ISI_RETURN_OK != ISIG_appMsg(chat_ptr, msg_ptr)) {
       ISID_unlockGChats();
       return (ISI_RETURN_FAILED);
   }
   ISID_unlockGChats();
   return (ISI_RETURN_OK);
}

/*
 * ======== ISI_disconnectChat() ========
 * This function is used to leave (disconnect from) a chat session.
 * If successfull then ISI will generate a ISI_EVENT_CHAT_DISCONNECTED event
 * indicating that any references to the chat ID are no longer valid.
 *
 * chatId :    The ID of the chat.  This is the value from calling
 *             ISI_initiateChat(), ISI_initiateGroupChat(), ISI_joinGroupChat()
 *             or from the ISI_EVENT_GROUP_CHAT_INITIATE or
 *             ISI_EVENT_CHAT_INITIATE event.
 *
 * Return Values:
 * ISI_RETURN_OK                   : The chat was disconnected.
 * ISI_RETURN_FAILED               : Failed to leave the chat session.
 * ISI_RETURN_INVALID_CHAT_ID      : The chat ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the chat linked list.
 */
ISI_Return ISI_disconnectChat(
    ISI_Id   chatId)
{
    ISID_GChatId *chat_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISI_RETURN_OK != ISID_lockGChats()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
        /* Then there is no active call */
        ISID_unlockGChats();
        return (ISI_RETURN_INVALID_CHAT_ID);
    }
    /* Now notify the protocol */
    if (NULL == (msg_ptr = ISIM_terminateChat(chat_ptr, NULL))) {
        ISID_unlockGChats();
        return (ISI_RETURN_FAILED);
    }
    if (ISI_RETURN_OK != ISIG_appMsg(chat_ptr, msg_ptr)) {
        ISID_unlockGChats();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockGChats();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_getChatHeader() ========
 * This function is used to get information about a chat session.
 * this routine is typically called when a request for a chat session has
 * been received.
 *
 * chatId :    The ID of the chat.  This is the value from calling
 *             ISI_initiateChat(), ISI_initiateGroupChat(), ISI_joinGroupChat()
 *             or from the ISI_EVENT_GROUP_CHAT_INITIATE or
 *             ISI_EVENT_CHAT_INITIATE event.
 *
 * subject_ptr : A pointer to a buffer.  If this routine is successful then ISI will
 *               write a NULL terminated string here indicating the 'subject' matter
 *               associated with this chat session.
 *
 * remoteAddress_ptr : A pointer to a buffer.  If this routine is successful then ISI will
 *               write a NULL terminated string here containing the address of the remote
 *               participant in the chat session.
 *
 * messageId_ptr : A pointer to an ISI_Id.  If this routine is successful then ISI will write
 *                 the ID of an instant message that was received if there was one included
 *                 with the chat request.  ISI_getMessageHeader() and ISI_readMessage() can
 *                 then be called to get the detials of the message included with the chat
 *                 session request.
 *
 * Return Values:
 * ISI_RETURN_OK                   : Success.
 * ISI_RETURN_FAILED               : Invalid parameter.
 * ISI_RETURN_INVALID_CHAT_ID      : The chat ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the chat linked list.
 */
ISI_Return ISI_getChatHeader(
    ISI_Id  chatId,
    char   *subject_ptr,
    char   *remoteAddress_ptr,
    ISI_Id *messageId_ptr)
{
    ISID_GChatId *chat_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!subject_ptr || !remoteAddress_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISI_RETURN_OK != ISID_lockGChats()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISI_RETURN_OK == ISID_gchatGet(chatId, &chat_ptr)) {
        OSAL_strncpy(remoteAddress_ptr, chat_ptr->szRemoteAddress, ISI_ADDRESS_STRING_SZ);
        OSAL_strncpy(subject_ptr, chat_ptr->szSubject, ISI_SUBJECT_STRING_SZ);
        *messageId_ptr = chat_ptr->firstMessageId;
        ISID_unlockGChats();
        return (ISI_RETURN_OK);
    }
    ISID_unlockGChats();
    return (ISI_RETURN_INVALID_CHAT_ID);
}

/*
 * ======== ISI_initiateGroupChatAdhoc() ========
 * This function is used to initiate an adhoc IM session using the service
 * specified by serviceId. ISI will generate ISI events indicating the state of
 * the session, for example: ISI_EVENT_CHAT_ACCEPTED,
 * ISI_EVENT_CHAT_DISCONNECTED, ISI_EVENT_CHAT_FAILED.
 *
 * chatId_ptr : A pointer to a unique identification number.  ISI will populate
 *              this value with a unique ID representing the this specific
 *              group chat session.  This value is used in subsequent ISI
 *              API's related to group chat sessions.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * participant_ptr : A mandatory null terminated string containing the address(s)
 *                   of the participant(s) that this entity wishes to invite to
 *                   the group chat session. If more than one participant is being
 *                   inviteed to the session then addresses are delimited with a
 *                   comma.  e.g. "sip:friendOne@domain.com, sip:friendTwo@domain.com"
 *
 * conferenceUri_ptr : An optional parameter than can contain a null terminated
 *               string specifying a URI with the Conference Factory address and
 *               Contribution ID parameter.  By providing this information, it
 *               is possible for a Group Chat that previously existed to in effect
 *               be re-created.  NOTE: It is re-created with the set of participants
 *               included in the participant_ptr parameter.
 *
 * subject_ptr : An optional parameter that can contain a null terminated string
 *               representing the 'subject' matter of the group chat
 *               conversation.
 *
 * sessionType : Indicate if this chat is use TLS.
 *
 * Return Values:
 * ISI_RETURN_OK                   : A request for a group chat sessions was successfully sent.
 * ISI_RETURN_FAILED               : Invalid parameter.
 * ISI_RETURN_INVALID_SERVICE_ID   : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the chat linked list.
 */
ISI_Return ISI_initiateGroupChatAdhoc(
    ISI_Id             *chatId_ptr,
    ISI_Id              serviceId,
    char               *participant_ptr,
    char               *conferenceUri_ptr,
    char               *subject_ptr,
    ISI_SessionType     sessionType)
{
    ISID_ServiceId *service_ptr;
    ISID_GChatId   *chat_ptr;
    ISI_Id          chatId;
    ISIP_Message   *msg_ptr;
    
    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Validate the mandatory params */
    if (!chatId_ptr || 
            !participant_ptr || 0 == *participant_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of group chat rooms */
    if (ISI_RETURN_OK != ISID_lockGChats()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISI_RETURN_OK == ISID_serviceGet(serviceId, &service_ptr)) {
        if (NULL == (chat_ptr = ISID_gchatCreate(service_ptr, conferenceUri_ptr,
                service_ptr->szNickname, service_ptr->szNickname, subject_ptr, participant_ptr, 0))) {
            ISID_unlockGChats();
            return (ISI_RETURN_FAILED);
        }
        else {
            /* Add it to the database.  Init the chatId pointer to zero
             * This indicates to the DB that we wish for the DB to determine
             * the Chat Id.
             */
            chatId = 0;
            if (ISI_RETURN_OK != ISID_gchatAdd(chat_ptr, &chatId)) {
                /* Free the memory and return */
                ISI_free(chat_ptr, ISI_OBJECT_GCHAT_ID);
                ISID_unlockGChats();
                return (ISI_RETURN_FAILED);
            }
            /* Now notify the protocol */
            if (NULL == (msg_ptr = ISIM_initiateChat(chat_ptr,
                    ISIP_GROUP_CHAT_REASON_INITIATE,
                    participant_ptr, NULL, sessionType))) {
                ISID_gchatDestroy(chatId);
                ISID_unlockGChats();
                return (ISI_RETURN_FAILED);
            }
            if (ISI_RETURN_OK != ISIG_appMsg(chat_ptr, msg_ptr)) {
                ISID_gchatDestroy(chatId);
                ISID_unlockGChats();
                return (ISI_RETURN_FAILED);
            }
            *chatId_ptr = chatId;
            ISID_unlockGChats();
            return (ISI_RETURN_OK);
        }
    }
    ISID_unlockGChats();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_inviteGroupChat() ========
 * This function is used to invite participants to a group chat session.
 *
 * chatId :    The ID of the chat session.  This is the value from calling
 *             ISI_initiateGroupChat(), ISI_joinGroupChat() or from the
 *             ISI_EVENT_GROUP_CHAT_INITIATE.
 *
 * participant_ptr : A mandatory null terminated string containing the address(s)
 *                   of the participant(s) that this entity wishes to invite.
 *                   addresses are delimited with a comma.
 *                   e.g. "sip:friendOne@domain.com, sip:friendTwo@domain.com"
 *
 * reason_ptr :  An optional parameter that can contain a null terminated string
 *               representing a reason (a.k.a. descriptive text) of the reason
 *               for the invitation. If supported by the underlying protocol,
 *               the remote party being invited will see this 'reason' in the
 *               invitation.
 *
 * Return Values:
 * ISI_RETURN_OK                   : The invitation was successfully sent.
 * ISI_RETURN_FAILED               : Failed to send the invitation.
 * ISI_RETURN_INVALID_CHAT_ID : The Group Chat Room ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the group chat linked list.
 */
ISI_Return ISI_inviteGroupChat(
    ISI_Id      chatId,
    char       *participant_ptr,
    char       *reason_ptr)
{
    ISID_GChatId *chat_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Validate the mandatory params */
    if (!participant_ptr || 0 == *participant_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISI_RETURN_OK != ISID_lockGChats()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
        /* Then there is no active call */
        ISID_unlockGChats();
        return (ISI_RETURN_INVALID_CHAT_ID);
    }

    /* Now notify the protocol */
    if (NULL == (msg_ptr = ISIM_inviteChat(chat_ptr, participant_ptr,
            reason_ptr))) {
        ISID_unlockGChats();
        return (ISI_RETURN_FAILED);
    }
    if (ISI_RETURN_OK != ISIG_appMsg(chat_ptr, msg_ptr)) {
        ISID_unlockGChats();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockGChats();
    return (ISI_RETURN_OK);
}


/*
 * ======== ISI_joinGroupChat() ========
 * This function is used to join an existing group chat session.
 * ISI will generate ISI events indicating the state of
 * the group chat room, ISI_EVENT_CHAT_ACCEPTED,
 * ISI_EVENT_CHAT_DISCONNECTED, ISI_EVENT_CHAT_FAILED.
 *
 * chatId_ptr : A pointer to a unique identification number.  ISI will populate
 *              this value with a unique ID representing the this specific
 *              group chat session.  This value is used in subsequent ISI
 *              API's related to group chat sessions.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 *
 * remoteAddress_ptr : A mandatory null terminated string containing the address of the
 *                    group chat sessions that this entity wishes to join.
 *
 * password_ptr :  An optional parameter that contains a null terminated
 *                string representing the password required to enter the group
 *                chat session.
 *
 * sessionType : Indicate if this chat is use TLS.
 *
 * Return Values:
 * ISI_RETURN_OK                   : A request to join a group chat sessions was successfully sent.
 * ISI_RETURN_FAILED               : Invalid parameters.
 * ISI_RETURN_INVALID_SERVICE_ID   : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the chat linked list.
 */
ISI_Return ISI_joinGroupChat(
    ISI_Id             *chatId_ptr,
    ISI_Id              serviceId,
    char               *remoteAddress_ptr,
    char               *password_ptr,
    ISI_SessionType     sessionType)
{
    ISID_ServiceId *service_ptr;
    ISID_GChatId   *chat_ptr;
    ISI_Id          chatId;
    ISIP_Message   *msg_ptr;
    vint            passwordRequired = 0;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Validate the other mandatory params */
    if (!chatId_ptr || !remoteAddress_ptr || 0 == *remoteAddress_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of group chat rooms */
    if (ISI_RETURN_OK != ISID_lockGChats()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISI_RETURN_OK == ISID_serviceGet(serviceId, &service_ptr)) {
        /*
         * Then we know the id is valid and active.
         */
        if (password_ptr && 0 != *password_ptr) {
            passwordRequired = 1;
        }
        if (NULL == (chat_ptr = ISID_gchatCreate(service_ptr,
                remoteAddress_ptr, service_ptr->szNickname, service_ptr->szNickname,
                NULL, NULL, passwordRequired))) {
            ISID_unlockGChats();
            return (ISI_RETURN_FAILED);
        }
        else {
            /* Add it to the database.  Init the chatId pointer to zero
             * This indicates to the DB that we wish for the DB to determine
             * the ChatId.
             */
            chatId = 0;
            if (ISI_RETURN_OK != ISID_gchatAdd(chat_ptr, &chatId)) {
                /* Free the memory and return */
                ISI_free(chat_ptr, ISI_OBJECT_GCHAT_ID);
                ISID_unlockGChats();
                return (ISI_RETURN_FAILED);
            }
            /* Now notify the protocol */
            if (NULL == (msg_ptr = ISIM_initiateChat(chat_ptr,
                    ISIP_GROUP_CHAT_REASON_JOIN, NULL, password_ptr,
                    sessionType))) {
                ISID_gchatDestroy(chatId);
                ISID_unlockGChats();
                return (ISI_RETURN_FAILED);
            }
            if (ISI_RETURN_OK != ISIG_appMsg(chat_ptr, msg_ptr)) {
                ISID_gchatDestroy(chatId);
                ISID_unlockGChats();
                return (ISI_RETURN_FAILED);
            }
            *chatId_ptr = chatId;
            ISID_unlockGChats();
            return (ISI_RETURN_OK);
        }
    }
    ISID_unlockGChats();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_kickGroupChat() ========
 * This function is used to kick a participant from a group chat session.  Note,
 * that kicking someone out of a group chat session typically requires
 * "administrative" permissions.  It also may only be possible if this entity
 *  is also the owner, or creator, of the group chat session.
 *
 * chatId :    The ID of the group chat session.  This is the value from calling
 *             ISI_initiateGroupChat(), ISI_joinGroupChat() or from the
 *             ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE event.
 *
 * participant_ptr : A mandatory null terminated string containing the address
 *                   of the participant to kick out.
 *
 * reason_ptr :  An optional parameter that can contain a null terminated string
 *               representing the reason (a.k.a. descriptive text) for kicking
 *               the participant from the group chat room. If supported by the
 *               underlying protocol, the remote party being kicked will see
 *               this 'reason'.
 *
 * Return Values:
 * ISI_RETURN_OK                   : The request to kick a participant was
 *                                   successfully sent.
 * ISI_RETURN_FAILED               : Failed to send the kick request.
 * ISI_RETURN_INVALID_CHAT_ID      : The  chat ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the chat linked list.
 */
ISI_Return ISI_kickGroupChat(
    ISI_Id   chatId,
    char    *participant_ptr,
    char    *reason_ptr)
{
    ISID_GChatId *chat_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

     /* Validate mandatory params */
    if (!participant_ptr || 0 == *participant_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISI_RETURN_OK != ISID_lockGChats()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
        /* Then there is no active call */
        ISID_unlockGChats();
        return (ISI_RETURN_INVALID_CHAT_ID);
    }

    /* Now notify the protocol */
    if (NULL == (msg_ptr = ISIM_kickChat(chat_ptr,
            participant_ptr, reason_ptr))) {
        ISID_unlockGChats();
        return (ISI_RETURN_FAILED);
    }
    if (ISI_RETURN_OK != ISIG_appMsg(chat_ptr, msg_ptr)) {
        ISID_unlockGChats();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockGChats();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_destroyGroupChat() ========
 * This function is used to destroy a group chat session.  If successfull then ISI
 * will generate a ISI_EVENT_CHAT_DISCONNECTED event indicating that any
 * references to the group chat ID are no longer valid. Note that the policy for
 * 'destroying' a group chat session is typically policed within the service providers
 * network.  If the entire session can not be 'destroyed' then at least this endpoint
 * connectivity to the group chat session is terminated.
 *
 * chatId :    The ID of the group chat session.  This is the value from calling
 *             ISI_initiateGroupChat(), ISI_joinGroupChat() or from the
 *             ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE event.
 *
 * reason_ptr :  An optional parameter that can contain a null terminated string
 *               representing the reason (a.k.a. descriptive text) for
 *               destroying the group chat room. Note that this is only
 *               supported if the underlying protocol supports this concept.
 *
 * Return Values:
 * ISI_RETURN_OK                   : The request to destroy a group chat session
 *                                   was successfully sent.
 * ISI_RETURN_FAILED               : Failed to send a request to destroy a group chat session.
 * ISI_RETURN_INVALID_CHAT_ID :      The chat ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the group chat linked list.
 */
ISI_Return ISI_destroyGroupChat(
    ISI_Id  chatId,
    char   *reason_ptr)
{
    ISID_GChatId *chat_ptr;
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISI_RETURN_OK != ISID_lockGChats()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
        /* Then there is no active call */
        ISID_unlockGChats();
        return (ISI_RETURN_INVALID_CHAT_ID);
    }
    /* Now notify the protocol */
    if (NULL == (msg_ptr = ISIM_destroyChat(chat_ptr, reason_ptr))) {
        ISID_unlockGChats();
        return (ISI_RETURN_FAILED);
    }
    if (ISI_RETURN_OK != ISIG_appMsg(chat_ptr, msg_ptr)) {
        ISID_unlockGChats();
        return (ISI_RETURN_FAILED);
    }
    ISID_unlockGChats();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_sendGroupChatMessage() ========
 * This function is used to send an instant message ('im') to chat session.
 * ISI will generate either a ISI_EVENT_MESSAGE_SEND_OK or
 * ISI_EVENT_MESSAGE_SEND_FAILED event indicating if the IM was successfully
 * send or not.
 *
 * msgId_ptr : A pointer to memory location where ISI will write a unique
 *             identification number representing this IM transaction.
 *
 * chatId :    The ID of the chat session.  This is the value from calling
 *             ISI_initiateChat(), ISI_initiateGroupChat(), ISI_joinGroupChat()
 *             or from the ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE
 *             event.
 *
 * msg_ptr : A null terminated string containing the im to send.
 *
 * report : An optional parameter containing a bitmap of reports to request
 *          (i.e. 'delivery' or 'display' reports).
 *
 * reportId_ptr : An optional parameter containing a report ID to associate with this
 *                instant message.
 *
 * Return Values:
 * ISI_RETURN_OK                   : The IM was successfully sent.
 * ISI_RETURN_FAILED               : Failed to send the IM.
 * ISI_RETURN_INVALID_CHAT_ID : The Group Chat Room ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the group chat linked list.
 */
ISI_Return ISI_sendChatMessage(
    ISI_Id             *msgId_ptr,
    ISI_Id              chatId,
    char               *msg_ptr,
    ISI_MessageReport   report,
    char               *reportId_ptr)
{
    ISID_TextId     *text_ptr;
    ISIP_Text       *t_ptr;
    vint             len;
    ISI_Id           textId;
    ISID_GChatId    *chat_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!msgId_ptr || !msg_ptr) {
        return (ISI_RETURN_FAILED);
    }

    if (report != ISI_MSG_RPT_NONE) {
        /* Then let's validate the id_ptr */
        if (!reportId_ptr) {
            return (ISI_RETURN_FAILED);
        }
    }

    /* Lock the database of texts */
    if (ISI_RETURN_OK != ISID_lockTexts()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
        /* Then there is no active group chat for this ID */
        ISID_unlockTexts();
        return (ISI_RETURN_INVALID_CHAT_ID);
    }

    /*
     * Very the state is active, note that both the file and group chat DB
     * is locked. So we are safe to test the state.
     */
    if (ISI_CHAT_STATE_ACTIVE != chat_ptr->state) {
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }

    /* The service Id is valid and the service is active */
    if (NULL == (text_ptr = ISID_textCreate())) {
        /* no available resources */
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }
    t_ptr = &text_ptr->isiMsg.msg.message;

    /* Copy the message */
    len = OSAL_strlen(msg_ptr);
    if (len >= (vint)sizeof(t_ptr->message)) {
        ISI_free(text_ptr, ISI_OBJECT_TEXT_ID);
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }
    OSAL_strncpy(t_ptr->message, msg_ptr, sizeof(t_ptr->message));
    text_ptr->msgLen = len;
    /* Copy the rest */
    text_ptr->service_ptr = chat_ptr->service_ptr;
    text_ptr->chatId = chatId;
    OSAL_strncpy(t_ptr->subject, chat_ptr->szSubject, ISI_SUBJECT_STRING_SZ);
    OSAL_strncpy(t_ptr->to, chat_ptr->szRemoteAddress, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(t_ptr->from, chat_ptr->szLocalIdentity, ISI_ADDRESS_STRING_SZ);

    /* Copy the reports that we are requesting for this text message */
    t_ptr->report = report;
    if (report != ISI_MSG_RPT_NONE) {
        /* Then we have an id value, so let's populate */
        OSAL_strncpy(t_ptr->messageId, reportId_ptr, ISI_ID_STRING_SZ);
    }
    /* Add it to the database.  Make sure msgId_ptr is NULL */
    textId = 0;
    if (ISI_RETURN_OK != ISID_textAdd(text_ptr, &textId)) {
        ISI_free(text_ptr, ISI_OBJECT_TEXT_ID);
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISI Message and send */
    ISIM_sendText(ISIP_TEXT_REASON_NEW, text_ptr, chat_ptr->service_ptr->e.desc, chatId);
    ISIT_appMsg(text_ptr, &text_ptr->isiMsg);
    *msgId_ptr = textId;
    ISID_unlockTexts();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_composingChatMessage() ========
 * This function is used to notify protocol layer that user is composing message.
 * Protocol layer sends local state via xml to remote party depends on the state
 * machine. There might not send any message regarding to this function, so
 * protocol layer will not generate any event for this function.
 *
 * chatId :    The ID of the chat session.  This is the value from calling
 *             ISI_initiateChat(), ISI_initiateGroupChat(), ISI_joinGroupChat()
 *             or from the ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE
 *             event.
 *
 * Return Values:
 * ISI_RETURN_OK                   : The IM was successfully sent.
 * ISI_RETURN_FAILED               : Failed to send the IM.
 * ISI_RETURN_INVALID_CHAT_ID : The Group Chat Room ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the group chat linked list.
 */
ISI_Return ISI_composingChatMessage(
    ISI_Id              chatId)
{
    ISID_TextId     *text_ptr;
    ISID_GChatId    *chat_ptr;
    ISIP_Text       *t_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of texts */
    if (ISI_RETURN_OK != ISID_lockTexts()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
        /* Then there is no active group chat for this ID */
        ISID_unlockTexts();
        return (ISI_RETURN_INVALID_CHAT_ID);
    }

    /*
     * Very the state is active, note that both the file and group chat DB
     * is locked. So we are safe to test the state.
     */
    if (ISI_CHAT_STATE_ACTIVE != chat_ptr->state) {
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }

    /* The service Id is valid and the service is active */
    if (NULL == (text_ptr = ISID_textCreate())) {
        /* no available resources */
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }
    text_ptr->service_ptr = chat_ptr->service_ptr;
    /* Populate an ISI Message and send */
    ISIM_sendText(ISIP_TEXT_REASON_COMPOSING_ACTIVE, text_ptr,
            chat_ptr->service_ptr->e.desc, chatId);
    /* add our identity to the message.  we should be using what was cached from the NOTIFY request
     * containing the conference event package info (RCS-e v1.2.2 Appendix M) */
    t_ptr = &text_ptr->isiMsg.msg.message;
    OSAL_strncpy(t_ptr->from, chat_ptr->szLocalIdentity, ISI_ADDRESS_STRING_SZ);
    OSAL_logMsg("%s:%d, Own Identity=%s\n", __FUNCTION__, __LINE__, t_ptr->from);

    ISIT_appMsg(text_ptr, &text_ptr->isiMsg);
    /*
     * Free the ISID_TextId. We don't look for responses to isComposing
     * updates so ISID_TextId never get's inserted into the DB list.
     */
    ISI_free(text_ptr, ISI_OBJECT_TEXT_ID);
    ISID_unlockTexts();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_sendMessageReport ========
 * Applications use this function to send 'delivery' or 'display' reports
 * for a message (im) within the context of a Chat (IM) session.
 *
 * ISI will generate a ISI_EVENT_MESSAGE_SEND_OK or ISI_EVENT_MESSAGE_SEND_FAILED
 * event indicating if the report was successfully sent or not.
 *
 * msgId_ptr : A pointer to memory location where ISI will write a unique
 *             identification number representing this message transaction.
 *
 * chatId :    The ID of the chat session.  This is the value from calling
 *             ISI_initiateChat(), ISI_initiateGroupChat(), ISI_joinGroupChat()
 *             or from the ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE
 *             event.
 *
 * report : A value representing the report being sent.  See ISI_MessageReport
 *          definition for all possible values.
 *
 * reportId_ptr : A null terminated string containing a ID value used to match
 *                an instant message and associated reports.  This should be
 *                the same reportId that was received when the original instant
 *                message was received.
 *
 *
 * Return Values:
 * ISI_RETURN_OK              : The message transaction has successfully
 *                              started.
 * ISI_RETURN_FAILED          : Function failed.  The message was not sent.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Text Message linked list.
 */
ISI_Return ISI_sendChatMessageReport(
    ISI_Id             *msgId_ptr,
    ISI_Id              chatId,
    ISI_MessageReport   report,
    char               *reportId_ptr)
{
    ISID_TextId     *text_ptr;
    ISIP_Text       *t_ptr;
    ISI_Id           textId;
    ISID_GChatId    *chat_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!msgId_ptr || !reportId_ptr || ISI_MSG_RPT_NONE == report) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of texts */
    if (ISI_RETURN_OK != ISID_lockTexts()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
        /* Then there is no active group chat for this ID */
        ISID_unlockTexts();
        return (ISI_RETURN_INVALID_CHAT_ID);
    }

    /*
     * Very the state is active, note that both the file and group chat DB
     * is locked. So we are safe to test the state.
     */
    if (ISI_CHAT_STATE_ACTIVE != chat_ptr->state) {
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }

    /* The service Id is valid and the service is active */
    if (NULL == (text_ptr = ISID_textCreate())) {
        /* no available resources */
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }
    t_ptr = &text_ptr->isiMsg.msg.message;

    text_ptr->msgLen = 0;
    text_ptr->service_ptr = chat_ptr->service_ptr;
    text_ptr->chatId = chatId;
    t_ptr->report = report;
    OSAL_strncpy(t_ptr->messageId, reportId_ptr, ISI_ID_STRING_SZ);
    OSAL_strncpy(t_ptr->subject, chat_ptr->szSubject, ISI_SUBJECT_STRING_SZ);
    OSAL_strncpy(t_ptr->to, chat_ptr->szRemoteAddress, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(t_ptr->from, chat_ptr->service_ptr->szUri,
            ISI_ADDRESS_STRING_SZ);

    /* Add it to the database.  Make sure msgId_ptr is NULL */
    textId = 0;
    if (ISI_RETURN_OK != ISID_textAdd(text_ptr, &textId)) {
        ISI_free(text_ptr, ISI_OBJECT_TEXT_ID);
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISI Message and send */
    ISIM_sendText(ISIP_TEXT_REASON_REPORT, text_ptr, chat_ptr->service_ptr->e.desc, chatId);
    ISIT_appMsg(text_ptr, &text_ptr->isiMsg);
    *msgId_ptr = textId;
    ISID_unlockTexts();
    return (ISI_RETURN_OK);
}


/*
 * ======== ISI_sendChatFile ========
 * Applications use this function to send a file within the context of
 * a chat session.  In other words this routine is used to send a file
 * within the SAME SIP SESSION that's used for the instant message traffic.
 *
 * During the file transfer, ISI may generate any of the following events:
 * ISI_EVENT_FILE_SEND_PROGRESS: Indicates a progress report of the file
 *                               transfer.
 * ISI_EVENT_FILE_SEND_PROGRESS_COMPLETED: Indicates that the file successfully
 *                                         completed transferring.
 * ISI_EVENT_FILE_SEND_PROGRESS_FAILED: The file transfer failed to complete.
 *
 * fileId_ptr : A pointer to memory location where ISI will write a unique
 *             identification number representing this file transfer transaction.
 *
 * chatId : The group chat identification number returned by ISI when
 *             ISI_initiateGroupChatXXX() was called
 *
 * subject_ptr : A pointer to memory location that has a null terminated
 *               string containing subject of transfer.  If subject_ptr is set
 *               to null, no subject will be include with the message.
 *
 * filePath_ptr : A null terminated string containing the absolute path of the
 *                file to send.
 *
 * fileType : The type of file being sent.  See the definintion of ISI_FileType
 *            for more details.
 *
 * Return Values:
 * ISI_RETURN_OK              : The file transaction transaction has
 *                              successfully started.
 * ISI_RETURN_FAILED          : Function failed.  The file was not sent.
 * ISI_RETURN_INVALID_CHAT_ID : The Group Chat Room ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the File Transfer linked list.
 */
ISI_Return ISI_sendChatFile(
    ISI_OUT ISI_Id       *fileId_ptr,
    ISI_INP ISI_Id        chatId,
    ISI_INP char         *subject_ptr,
    ISI_INP char         *filePath_ptr,
    ISI_INP ISI_FileType  fileType)
{
    ISID_FileId     *file_ptr;
    ISIP_File       *f_ptr;
    ISI_Id           fileId;
    ISID_GChatId    *chat_ptr;
    vint             len;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!fileId_ptr || !filePath_ptr) {
        return (ISI_RETURN_FAILED);
    }

    if (ISI_FILETYPE_INVALID <= fileType) {
        return (ISI_RETURN_FAILED);
    }

    len = OSAL_strlen(filePath_ptr);
    if (len >= ISI_FILE_PATH_STRING_SZ) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of texts */
    if (ISI_RETURN_OK != ISID_lockFiles()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
        /* Then there is no active group chat for this ID */
        ISID_unlockFiles();
        return (ISI_RETURN_INVALID_CHAT_ID);
    }

    /*
     * Very the state is active, note that both the file and group chat DB
     * is locked. So we are safe to test the state.
     */
    if (ISI_CHAT_STATE_ACTIVE != chat_ptr->state) {
        ISID_unlockFiles();
        return (ISI_RETURN_FAILED);
    }

    /* The service Id is valid and the service is active */
    if (NULL == (file_ptr = ISID_fileCreate())) {
        /* no available resources */
        ISID_unlockFiles();
        return (ISI_RETURN_FAILED);
    }

    file_ptr->service_ptr = chat_ptr->service_ptr;
    file_ptr->state = ISID_FILE_STATE_SEND;

    f_ptr = &file_ptr->isiMsg.msg.file;

    OSAL_strncpy(f_ptr->filePath, filePath_ptr, sizeof(f_ptr->filePath));
    f_ptr->fileType = fileType;
    f_ptr->chatId = chatId;
    if (NULL != subject_ptr) {
        OSAL_strncpy(f_ptr->subject, subject_ptr, ISI_SUBJECT_STRING_SZ);
    }
    OSAL_strncpy(f_ptr->to, chat_ptr->szRemoteAddress, ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(f_ptr->from, chat_ptr->service_ptr->szUri,
            ISI_ADDRESS_STRING_SZ);

    /* Add it to the database.  Make sure msgId_ptr is NULL */
    fileId = 0;
    if (ISI_RETURN_OK != ISID_fileAdd(file_ptr, &fileId)) {
        ISI_free(file_ptr, ISI_OBJECT_FILE_ID);
        ISID_unlockFiles();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISI Message and send */
    ISIM_sendFile(file_ptr, chat_ptr->service_ptr->e.desc);
    ISIFT_appMsg(file_ptr, &file_ptr->isiMsg);
    *fileId_ptr = fileId;
    ISID_unlockFiles();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_sendPrivateGroupChatMessage() ========
 * NOTE: THIS ROUTINE IS CURRENTLY UNSUPPORTED
 * This function is used to send a private message to a specific participant
 * in the group chat room.  If the im is successfully sent then ISI will
 * generate either a ISI_EVENT_MESSAGE_SEND_OK or ISI_EVENT_MESSAGE_SEND_FAILED
 * event indicating if the IM was succsfully delived to the participant.
 *
 * msgId_ptr : A pointer to memory location where ISI will write a unique
 *             identification number representing this IM transaction.
 *
 * chatId :    The ID of the group chat room.  This is the value from calling
 *             ISI_initiateGroupChat(), ISI_joinGroupChat() or from the
 *             ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE event.
 *
 * participant_ptr : A mandatory null terminated string containing the address
 *                   of the participant to send the private message to.
 *
 * msg_ptr : A null terminated string containing the text body of the message
 *           to send.
 *
 * Return Values:
 * ISI_RETURN_OK                   : The IM was successfully sent.
 * ISI_RETURN_FAILED               : Failed to send the IM.
 * ISI_RETURN_INVALID_SERVICE_ID   : The Service ID is invalid.
 * ISI_RETURN_INVALID_CHAT_ID : The Group Chat Room ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the chat linked list.
 */
ISI_Return ISI_sendPrivateGroupChatMessage(
    ISI_Id     *msgId_ptr,
    ISI_Id      chatId,
    char       *partipant_ptr,
    char       *msg_ptr)
{
    ISID_TextId     *text_ptr;
    ISIP_Text       *t_ptr;
    vint             len;
    ISI_Id           textId;
    ISID_GChatId    *chat_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!msgId_ptr || !msg_ptr || !partipant_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of texts */
    if (ISI_RETURN_OK != ISID_lockTexts()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
        /* Then there is no active group chat for this ID */
        ISID_unlockTexts();
        return (ISI_RETURN_INVALID_CHAT_ID);
    }

    /*
     * Very the state is active, note that both the text and group chat DB
     * is locked. So we are safe to test the state.
     */
    if (ISI_CHAT_STATE_ACTIVE != chat_ptr->state) {
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }

    /* The service Id is valid and the service is active */
    if (NULL == (text_ptr = ISID_textCreate())) {
        /* no available resources */
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }
    t_ptr = &text_ptr->isiMsg.msg.message;

    /* Copy the message */
    len = OSAL_strlen(msg_ptr);
    if (len >= (vint)sizeof(t_ptr->message)) {
        ISI_free(text_ptr, ISI_OBJECT_TEXT_ID);
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }
    OSAL_strncpy(t_ptr->message, msg_ptr, sizeof(t_ptr->message));
    text_ptr->msgLen = len;
    /* Copy the rest */
    text_ptr->service_ptr = chat_ptr->service_ptr;
    text_ptr->chatId = chatId;
    OSAL_strncpy(t_ptr->to, partipant_ptr, ISI_ADDRESS_STRING_SZ);
    /* Add it to the database.  Make sure msgId_ptr is NULL */
    textId = 0;
    if (ISI_RETURN_OK != ISID_textAdd(text_ptr, &textId)) {
        ISI_free(text_ptr, ISI_OBJECT_TEXT_ID);
        ISID_unlockTexts();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISI Message and send */
    ISIM_sendText(ISIP_TEXT_REASON_NEW, text_ptr, chat_ptr->service_ptr->e.desc, chatId);
    ISIT_appMsg(text_ptr, &text_ptr->isiMsg);
    *msgId_ptr = textId;
    ISID_unlockTexts();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_sendGroupChatPresence() ========
 * This function is used to send a presence updates for this entity to a
 * group chat room.  It is possible to have a different presence state
 * displayed in a group chat room than what typical subscribers to presence see.
 * This routine allows users to set the presence state as seen by others that
 * are particpating in a group chat room.
 * If the presence update is successfully sent then ISI will generate either a
 * ISI_EVENT_PRE_SEND_OK or ISI_EVENT_PRES_SEND_FAILED event indicating
 * that the presence state failed to be u pdated int he group chat room.
 *
 * presId_ptr : A pointer to memory location where ISI will write a unique
 *             identification number representing this presence transaction.
 *
 * chatId :    The ID of the group chat room.  This is the value from calling
 *             ISI_initiateGroupChat(), ISI_joinGroupChat() or from the
 *             ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE event.
 *
 * presence_ptr : A null terminated string containing the protocol specific
 *                presence string.  This string represents the entities actual
 *                state of presence. Typically the format of this string is XML.
 *
 * Return Values:
 * ISI_RETURN_OK                   : Presence was successfully sent.
 * ISI_RETURN_FAILED               : Failed to update the presence state.
 * ISI_RETURN_INVALID_SERVICE_ID   : The Service ID is invalid.
 * ISI_RETURN_INVALID_CHAT_ID : The Group Chat Room ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI is not initialized.
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the chat linked list.
 */
ISI_Return ISI_sendGroupChatPresence(
    ISI_Id *presId_ptr,
    ISI_Id  chatId,
    char   *presence_ptr)
{
    ISIP_Presence   *p_ptr;
    ISID_PresId     *pres_ptr;
    ISI_Id           presId;
    ISID_GChatId    *chat_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Check for required parameters */
    if (!presId_ptr || !presence_ptr || 0 == *presence_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISI_RETURN_OK != ISID_lockPres()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
        /* Then there is no active group chat for this ID */
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_CHAT_ID);
    }

    /*
     * Very the state is active, note that both the pres and group chat DB
     * is locked. So we are safe to test the state.
     */
    if (ISI_CHAT_STATE_ACTIVE != chat_ptr->state) {
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }

    if (NULL == (pres_ptr = ISID_presCreate())) {
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }

    p_ptr = &pres_ptr->isiMsg.msg.presence;
    pres_ptr->service_ptr = chat_ptr->service_ptr;
    pres_ptr->chatId = chatId;
    pres_ptr->reason = ISIP_PRES_REASON_PRESENCE;
    OSAL_strncpy(p_ptr->presence, presence_ptr, ISI_PRESENCE_STRING_SZ);

    /* This will force the DB sub module to assign an id */
    presId = 0;
    if (ISI_RETURN_OK != ISID_presAdd(pres_ptr, &presId)) {
        /* Free the memory and return */
        ISI_free(pres_ptr, ISI_OBJECT_PRES_ID);
        ISID_unlockPres();
        return (ISI_RETURN_FAILED);
    }
    /* Populate an ISI Message and notify the protocol */
    ISIM_pres(pres_ptr, ISIP_PRES_REASON_PRESENCE, chat_ptr->szRemoteAddress);
    ISIR_appMsg(pres_ptr, &pres_ptr->isiMsg);
    *presId_ptr = presId;
    ISID_unlockPres();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_readGroupChatPresence() ========
 * Applications call this function to read group chat presence information of an entity
 * when a ISI event ISI_EVENT_GROUP_CHAT_PRES_RECEIVED is received by the application.
 *
 * If the 'len' is smaller than the length of the presence information, ISI
 * will place partial presence information in this buffer.  Application must
 * call ISI_readGroupChatPresence() repeatedly to receive complete presence information.
 * When ISI has delivered the complete presence state information to ISI, it
 * will return ISI_RETURN_DONE.
 *
 * presId    : A unique identification number representing the presence
 *             transaction entry to retrieve.
 *
 * chatId_ptr : a pointer to memory location where ISI will write the group chat
 *              identification number for the chat room in whose context this
 *              presence was received.
 *
 * from_ptr :  A pointer to a buffer where ISI will write a string representing
 *             the entity who's presence state is being signalled. the size of
 *             the buffer pointer to by the value MUST be ISI_ADDRESS_STRING_SZ
 *             bytes long.
 *
 * presence_ptr : A pointer to memory location where ISI will write presence
 *                information as a null terminated string.
 *
 * len : The size of the buffer pointed to by presence_ptr.  Note, if the length
 *       of this buffer is too small for ISI to write the entire presence string
 *       then ISI will write a portion of the presence string are return
 *       ISI_RETURN_OK. In this case, the application MUST call this routine
 *       repeatedly until ISI_RETURN_DONE is returned to ensure the entire
 *       presence string was read.
 *
 * Return Values:
 * ISI_RETURN_OK                 : Presence information returned. But there is
 *                                 still more to be read.
 * ISI_RETURN_DONE               : All Presence information has been returned.
 * ISI_RETURN_FAILED             : Presence information not found.
 * ISI_RETURN_INVALID_PRESENCE_ID: The presId parameter was invalid.
 * ISI_RETURN_NOT_INIT           : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Presence linked list.
 */
ISI_Return ISI_readGroupChatPresence(
    ISI_Id  presId,
    ISI_Id *chatId_ptr,
    char   *from_ptr,
    char   *presence_ptr,
    int     len)
{
    ISID_PresId   *pres_ptr;
    ISIP_Presence *p_ptr;
    vint           strLen;
    char          *str_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!presence_ptr || !from_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database */
    if (ISID_lockPres() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the presence transaction via the presId */
    if (ISID_presGet(presId, &pres_ptr) != ISI_RETURN_OK) {
        ISID_unlockPres();
        return (ISI_RETURN_INVALID_PRESENCE_ID);
    }

    p_ptr = &pres_ptr->isiMsg.msg.presence;

    /* Set the from */
    OSAL_strncpy(from_ptr, p_ptr->from, ISI_ADDRESS_STRING_SZ);

    str_ptr = p_ptr->presence + pres_ptr->presOffset;
    strLen  = pres_ptr->presLen - pres_ptr->presOffset;
    OSAL_strncpy(presence_ptr, str_ptr, len);
    *chatId_ptr = pres_ptr->chatId;
    if (strLen >= len) {
        /*
         * Then the output buffer was too short for the whole thing
         * update the position in the from buffer
         */
        len--;
        pres_ptr->presOffset += len;
        ISID_unlockPres();
        return (ISI_RETURN_OK);
    }

    /* The whole presence action has been written so free up the resource */
    ISID_presDestroy(presId);
    ISID_unlockPres();
    return (ISI_RETURN_DONE);
}

/*
 * ======== ISI_getGroupChatHeader() ========
 * This function is used to get the information of a group chat session.
 * This routine is typically called after an application receives an
 * ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE event.
 * It can be used to see the 'details' of the invitation to a group chat
 * session.
 *
 * chatId :    The ID of the group chat room.  This is the value from the
 *             ISI_EVENT_GROUP_CHAT_INITIATE or ISI_EVENT_CHAT_INITIATE event.
 *
 * subject_ptr : A pointer to a buffer where ISI will write the subject
 *               associated with this chat room.  This buffer MUST be at least
 *               ISI_SUBJECT_STRING_SZ bytes long.
 *
 * conferenceUri_ptr : A pointer to a buffer where ISI will write the address of
 *                     the conference factory used to manage the group chat.
 *                     This buffer MUST be at least ISI_ADDRESS_STRING_SZ bytes long.
 *
 * participants_ptr : A pointer to a buffer where ISI will write the addess(s) of
 *               the other particupants already in the group chat.
 *               This buffer MUST be at least ISI_LONG_ADDRESS_STRING_SZ bytes long.
 *
 * Return Values:
 * ISI_RETURN_OK                   : Success.  Group chat session information was
 *                                   returned.
 * ISI_RETURN_FAILED               : Failed to retrieve group chat session info.
 * ISI_RETURN_INVALID_CHAT_ID      : The Chat ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the chat linked list.
 */
ISI_Return ISI_getGroupChatHeader(
    ISI_Id  chatId,
    char   *subject_ptr,
    char   *conferenceUri_ptr,
    char   *participants_ptr)
{
    ISID_GChatId *chat_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!subject_ptr || !conferenceUri_ptr || !participants_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISI_RETURN_OK != ISID_lockGChats()) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    if (ISI_RETURN_OK == ISID_gchatGet(chatId, &chat_ptr)) {
        /*
         * the conference URI will also include the contribution ID
         * as a parameter
         */
        OSAL_snprintf(conferenceUri_ptr, ISI_ADDRESS_STRING_SZ, "%s;%s=%s",
                chat_ptr->szRemoteAddress, ISI_CONTRIBUTION_ID_PARAM_STR, 
                chat_ptr->szContributionId);
        OSAL_strncpy(subject_ptr, chat_ptr->szSubject, ISI_SUBJECT_STRING_SZ);
        OSAL_strncpy(participants_ptr, chat_ptr->szParticipants, ISI_LONG_ADDRESS_STRING_SZ);
        ISID_unlockGChats();
        return (ISI_RETURN_OK);
    }
    ISID_unlockGChats();
    return (ISI_RETURN_INVALID_CHAT_ID);
}

/*
 * ======== ISI_getChatState() ========
 * This function is used to get the state of a chat session.
 *
 * chatId :    The ID of the chat session.  This is the value from calling
 *             ISI_initiateChat(), ISI_initiateGroupChat(), ISI_joinGroupChat()
 *             or from the ISI_EVENT_GROUP_CHAT_INITIATE or
 *             ISI_EVENT_CHAT_INITIATE event.
 *
 * chatState_ptr : A pointer to an emerated value.  If this routine is
 *                 successful then ISI will write the state of the specified
 *                 group chat room.  Possible values are:
 *                  ISI_CHAT_STATE_INITIATING,
 *                  ISI_CHAT_STATE_ACTIVE,
 *                  ISI_CHAT_STATE_INVALID
 *
 * Return Values:
 * ISI_RETURN_OK                   : Success.  Chat state was  returned.
 * ISI_RETURN_FAILED               : Failed to get the chat state.
 * ISI_RETURN_INVALID_CHAT_ID      : The Chat ID is invalid.
 * ISI_RETURN_NOT_INIT             : ISI not initialized
 * ISI_RETURN_MUTEX_ERROR          : Could not lock the chat linked list.
 */
ISI_Return ISI_getChatState(
    ISI_Id              chatId,
    ISI_ChatState *chatState_ptr)
{
    ISID_GChatId *chat_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!chatState_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of calls */
    if (ISID_lockGChats() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_gchatGet(chatId, &chat_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        ISID_unlockGChats();
        return (ISI_RETURN_INVALID_CHAT_ID);
    }
    *chatState_ptr = chat_ptr->state;
    ISID_unlockGChats();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_mediaControl() ========
 * This function is used to command the media engine.  Applications typically
 * use this function to indicate that the device is in "speaker phone" mode.
 * The media engine needs to know the speaker phone mode to setup the AEC
 * appropriately.
 *
 * cmd = The command to issue to the media engine.
 * arg = an argument to the media cmd.
 *
 * Return Values:
 * ISI_RETURN_OK              : The command was successfully issued.
 * ISI_RETURN_FAILED          : The command failed.
 * ISI_RETURN_NOT_SUPPORTED   : The command is not supported.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 */
ISI_Return ISI_mediaControl(
    ISI_MediaControl cmd,
    int              arg)
{
    ISIP_Message *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Verify the tone is valid. */
    if (ISI_MEDIA_CNTRL_LAST <= cmd) {
        return (ISI_RETURN_NOT_SUPPORTED);
    }

    /* Populate an ISIP Message and notify the protocol */
    if (NULL == (msg_ptr = ISIM_media(cmd, arg))) {
        return (ISI_RETURN_FAILED);
    }
    ISIM_appMsg(msg_ptr);
    return (ISI_RETURN_OK);
}

/* Pete1 to remove in future maybe?? */
ISI_Return ISI_getServiceAtribute(
    ISI_OUT ISI_Id              *evtId_ptr,
    ISI_INP ISI_Id               serviceId,
    ISI_INP ISI_SeviceAttribute  cmd,
    ISI_INP char                *arg1_ptr,
    ISI_INP char                *arg2_ptr)
{
    ISI_Return      ret;
    ISID_ServiceId *service_ptr;
    ISID_EvtId     *evt_ptr;
    ISI_Id          evtId;
                    
    if (!ISID_getState()) {
        OSAL_logMsg("%s:%d : FAIL\n", __FUNCTION__, __LINE__);        
        return (ISI_RETURN_NOT_INIT);
    }

    if (!evtId_ptr) {
        OSAL_logMsg("%s:%d : FAIL\n", __FUNCTION__, __LINE__);        
        return (ISI_RETURN_FAILED);
    }

    /* Validate the ussd_ptr */
    if (!arg1_ptr || !arg2_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of events */
    if (ISID_lockEvts() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }
    
    ret = ISID_serviceGet(serviceId, &service_ptr);
    if (ret == ISI_RETURN_FAILED) {
        OSAL_logMsg("%s : FAIL do serviceGet , id=%d\n", 
                __FUNCTION__, serviceId);
        /* Then the service does not even exist */
        ISID_unlockEvts();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    if (ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
        /* Then the service ID is valid but the service is deactivated */
        ISID_unlockEvts();
        OSAL_logMsg("%s:%d : FAIL\n", __FUNCTION__, __LINE__);
        return (ISI_RETURN_FAILED);
    }
    
    /* If we are here then the service is active */
    if ((evt_ptr = ISID_evtCreate()) == NULL) {
        ISID_unlockEvts();
        return (ISI_RETURN_FAILED);
    }
    evt_ptr->service_ptr = service_ptr;

    /*
     * Before you add this object, make sure that evtId is zero.
     * This will force the DB sub module to assign an id
     */
    evtId = 0;
    if (ISID_evtAdd(evt_ptr, &evtId) == ISI_RETURN_FAILED) {
        /* Free the memory and return */
        ISI_free(evt_ptr, ISI_OBJECT_EVT_ID);
        ISID_unlockEvts();
        OSAL_logMsg("%s:%d : FAIL\n", __FUNCTION__, __LINE__);        
        return (ISI_RETURN_FAILED);
    }

    /* Populate a ISIP Message and notify the protocol */
    ISIM_getServiceAttribute(evt_ptr, cmd, arg1_ptr, arg2_ptr);
    ISIE_appMsg(evt_ptr, &evt_ptr->isiMsg);
    *evtId_ptr = evtId;
    ISID_unlockEvts();
    
    return (ISI_RETURN_OK);    
}

/*
 * ======== ISI_setAkaAuthResp() ========
 * This function is used set resp, ck and ik for ISIM authentication response.
 *
 * serviceId : This is the service identification number returned by ISI when
 *            ISI_allocService() was called.
 *
 * result: The result of aka response.
 *
 * resp_ptr : A null terminated string containing the resp
 *
 * resLength : The length of response that can vary between 32 and 128 bits.
 *
 * auts_ptr : A null terminated string containing the auts
 *
 * ck_ptr : A null terminated string containing the ck
 *
 * ik_ptr : A null terminated string containing the ik
 *
 * Return Values:
 * ISI_RETURN_OK                  : Assigned/changed successfully.
 * ISI_RETURN_INVALID_SERVICE_ID  : Service ID is invalid.
 * ISI_RETURN_NOT_INIT            : ISI not initialized
 * ISI_RETURN_FAILED              : Could not notify the protocol
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the Service linked list.
 *
 */
ISI_Return ISI_setAkaAuthResp(
    ISI_Id  serviceId,
    int     result,
    char   *resp_ptr,
    int     resLength,
    char   *auts_ptr,
    char   *ck_ptr,
    char   *ik_ptr)
{
    ISID_ServiceId  *service_ptr;
    ISIP_Message    *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /*
     * Verify all parameters are NOT NULL.
     */
    if (!resp_ptr || !auts_ptr || !ck_ptr || !ik_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Check the serviceId it may or may not be active.  But we don't care.*/
    if (ISID_serviceGet(serviceId, &service_ptr) == ISI_RETURN_FAILED) {
        /* Then the service does not exist */
        ISID_unlockServices();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    /* Then the id is valid */
    if ((msg_ptr = ISIM_setAkaAuthResp(service_ptr, result, resp_ptr, resLength,
            auts_ptr, ck_ptr, ik_ptr)) == NULL) {
        ISID_unlockServices();
        return (ISI_RETURN_FAILED);
    }

    ISIS_appMsg(msg_ptr);
    ISID_unlockServices();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_getAkaAuthChallenge() ========
 * This function is used to get rand and autn for AKA authentication challenge.
 *
 * serviceId : This is the service identification number returned by ISI when
 *            ISI_allocService() was called.
 *
 * rand_ptr : A pointer a memory location that will be populate with the rand.
              The size of populated data is ISI_AKA_AUTH_RAND_STRING_SZ.
 *
 * autn_ptr : A pointer a memory location that will be populate with the autn.
              The size of populated data is ISI_AKA_AUTH_AUTN_STRING_SZ.
 *
 * Return Values:
 * ISI_RETURN_OK                  : Assigned/changed successfully.
 * ISI_RETURN_INVALID_SERVICE_ID  : Service ID is invalid.
 * ISI_RETURN_NOT_INIT            : ISI not initialized
 * ISI_RETURN_FAILED              : Could not notify the protocol
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the Service linked list.
 *
 */
ISI_Return ISI_getAkaAuthChallenge(
    ISI_Id  serviceId,
    char   *rand_ptr,
    char   *autn_ptr)
{
    ISID_ServiceId  *service_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /*
     * Verify all parameters are NOT NULL.
     */
    if (!rand_ptr || !autn_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Check the serviceId it may or may not be active.  But we don't care.*/
    if (ISID_serviceGet(serviceId, &service_ptr) == ISI_RETURN_FAILED) {
        /* Then the service does not exist */
        ISID_unlockServices();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    /* Then the id is valid, get rand and autn from db */
    if (ISI_RETURN_OK != ISID_akaChallengeGet(service_ptr, rand_ptr, autn_ptr)) {
        /* Failed to get aka challenge */
        ISID_unlockServices();
        return (ISI_RETURN_FAILED);
    }

    ISID_unlockServices();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_serviceGetIpsec() ========
 * This function is used to get current IPSec protect ports and SPI.
 *
 * serviceId : This is the service identification number returned by ISI when
 *            ISI_allocService() was called.
 *
 * portUc_ptr:  A pointer to a integer where ISI will write the protected
 *              UE client port.
 * 
 * portUs_ptr:  A pointer to a integer where ISI will write the protected
 *              UE server port.
 * 
 * portPc_ptr:  A pointer to a integer where ISI will write the protected
 *              proxy client port.
 * 
 * portPs_ptr:  A pointer to a integer where ISI will write the protected
 *              proxy server port.
 * 
 * spiUc_ptr:  A pointer to a integer where ISI will write the protected
 *              UE client spi.
 * 
 * spitUs_ptr:  A pointer to a integer where ISI will write the protected
 *              UE server spi.
 * 
 * spiPc_ptr:  A pointer to a integer where ISI will write the protected
 *              proxy client spi.
 * 
 * spiPs_ptr:  A pointer to a integer where ISI will write the protected
 *              proxy server spi.
 * 
 * Return Values:
 * ISI_RETURN_OK                  : Assigned/changed successfully.
 * ISI_RETURN_INVALID_SERVICE_ID  : Service ID is invalid.
 * ISI_RETURN_NOT_INIT            : ISI not initialized
 * ISI_RETURN_FAILED              : Could not notify the protocol
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the Service linked list.
 *
 */
ISI_Return ISI_serviceGetIpsec(
    ISI_Id  serviceId,
    int    *portUc_ptr,
    int    *portUs_ptr,
    int    *portPc_ptr,
    int    *portPs_ptr,
    int    *spiUc_ptr,
    int    *spiUs_ptr,
    int    *spiPc_ptr,
    int    *spiPs_ptr)
{
    ISID_ServiceId  *service_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /*
     * Verify all parameters are NOT NULL.
     */
    if (!portUc_ptr || !portUs_ptr || !portPc_ptr ||
            !portPs_ptr || !spiUc_ptr || !spiUs_ptr || !spiPc_ptr ||
            !spiPs_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Check the serviceId it may or may not be active.  But we don't care.*/
    if (ISID_serviceGet(serviceId, &service_ptr) == ISI_RETURN_FAILED) {
        /* Then the service does not exist */
        ISID_unlockServices();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    /* Then the id is valid, get rand and autn from db */
    *portUc_ptr = service_ptr->portUc;
    *portUs_ptr = service_ptr->portUs;
    *portPc_ptr = service_ptr->portPc;
    *portPs_ptr = service_ptr->portPs;
    *spiUc_ptr  = service_ptr->spiUc;
    *spiUs_ptr  = service_ptr->spiUs;
    *spiPc_ptr  = service_ptr->spiPc;
    *spiPs_ptr  = service_ptr->spiPs;

    ISID_unlockServices();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_diagAudioRecord() ========
 * This function is used to do a audio record for diagnostic purpose.
 *
 * file_ptr : The file name with full path to do the audio record.
 *
 * Return Values:
 * ISI_RETURN_OK                : Set the diagnostic enable/disable is OK.
 * ISI_RETURN_NOT_INIT          : ISI layer not Initialized
 * ISI_RETURN_MUTEX_ERROR       : Could not lock the Service linked list.
 */

ISI_Return ISI_diagAudioRecord(
    ISI_INP char   *file_ptr)
{
    ISIP_Message   *msg_ptr;
    vint            x = 0;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Send audio record request to all protocol queue. */
    while (x < ISID_MAX_NUM_PROTOCOLS) {
        if (ISID_checkProtocol(x) == ISI_RETURN_OK) {
            msg_ptr = ISIM_diagAudioRecord(x, file_ptr);
            if (msg_ptr) {
                ISIDIAG_appMsg(msg_ptr);
            }
        }
        x++;
    }

    ISID_unlockServices();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_diagAudioPlay() ========
 * This function is used to do a audio play for diagnostic purpose.
 *
 * file_ptr : The file name with full path to do the audio play.
 *
 * Return Values:
 * ISI_RETURN_OK                : Set the diagnostic enable/disable is OK.
 * ISI_RETURN_NOT_INIT          : ISI layer not Initialized
 * ISI_RETURN_MUTEX_ERROR       : Could not lock the Service linked list.
 */

ISI_Return ISI_diagAudioPlay(
    ISI_INP char   *file_ptr)
{
    ISIP_Message   *msg_ptr;
    vint            x = 0;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Send audio play request to all protocol queue. */
    while (x < ISID_MAX_NUM_PROTOCOLS) {
        if (ISID_checkProtocol(x) == ISI_RETURN_OK) {
            msg_ptr = ISIM_diagAudioPlay(x, file_ptr);
            if (msg_ptr) {
                ISIDIAG_appMsg(msg_ptr);
            }
        }
        x++;
    }

    ISID_unlockServices();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_getNextService() ========
 * This function is used to get next valid service.
 *
 * serviceId_ptr : A pointer to an integer which indicates the service id.
 *            This function will search the database for next valid service
 *            from the given service id. This fuction returns ISI_RETURN_FAILED
 *            If no valid service retrieved.
 *
 * protocol_ptr : A pointer to an integer which indicates the protocol of the
 *            service.
 *
 * isEmergency_ptr : A pointer to a integer which indicates if the service is
 *            an emergency service.
 *
 * features_ptr: A pointer to integer contains the feature bit mask a service
 *             supported.
 *
 * isActivated_ptr: A pointer to integer which indicates if the service is been
 *             activated.
 *
 * Return Values:
 * ISI_RETURN_OK                  : Next service retrived successfully.
 * ISI_RETURN_NOT_INIT            : ISI not initialized
 * ISI_RETURN_FAILED              : Null serviceId_ptr or failed to find next
 *                                  active service.
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the Service linked list.
 */
ISI_Return ISI_getNextService(
    ISI_IO  ISI_Id *serviceId_ptr,
    ISI_OUT int    *protocol_ptr,
    ISI_OUT int    *isEmergency_ptr,
    ISI_OUT int    *features_ptr,
    ISI_OUT int    *isActivated_ptr)
{
    ISID_ServiceId *service_ptr;
    vint            idx;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (NULL == serviceId_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Take a mutex against the services database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        ISI_free(service_ptr, ISI_OBJECT_SERVICE_ID);
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Loop to get next service */
    for (idx = (*serviceId_ptr) + 1; idx <  ISID_MAX_NUM_SERVICES; idx++) {
        if (ISID_serviceGet(idx, &service_ptr) != ISI_RETURN_FAILED) {
            /*
             * Then we know the id is valid.
             * It may or not be 'activated' but we don't care
             */
            *serviceId_ptr = idx;
            *protocol_ptr = service_ptr->protocol;
            *isEmergency_ptr = service_ptr->isEmergency;
            *features_ptr = service_ptr->features;
            *isActivated_ptr = service_ptr->isActivated;
            ISID_unlockServices();
            return (ISI_RETURN_OK);
        }
    }
    /* Cannot find valid service */
    ISID_unlockServices();
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISI_setFeature() ========
 * This function is to set required feature to ISI server to indicate those
 * feature related event should route to ISI client.
 *
 * features: Bit mask of ISI_FeatureType.
 *
 * Return Values:
 * ISI_RETURN_FAILED : This function only implemeted in ISI server, it's not
 *                     required and supported in ISI.
 */
ISI_Return ISI_setFeature(
    ISI_INP int features)
{
    /* Always return fail */
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISI_serviceSetFeature() ========
 * This function is to set feature(s) to a service to indicate which
 * feature a service support.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * features: Bit mask of ISI_FeatureType.
 *
 * Return Values:
 * ISI_RETURN_OK                 : The radio interface was set for the service.
 * ISI_RETURN_INVALID_SERVICE_ID : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI is not initialized
 * ISI_RETURN_FAILED             : Could not convert the IP address specified in address_ptr.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceSetFeature(
    ISI_INP ISI_Id serviceId,
    ISI_INP int    features)
{
    ISID_ServiceId *service_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid.
         * It may or not be 'activated' but we don't care.
         */
        service_ptr->features = features;
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_serviceGetFeature() ========
 * This function is to get feature(s) that a service support.
 *
 * serviceId : This is the service identification number returned by ISI when
 *             ISI_allocService() was called.
 *
 * features_ptr: A pointer to integer contains the feature bit mask a service
 *             supported.
 *
 * Return Values:
 * ISI_RETURN_OK                 : The radio interface was set for the service.
 * ISI_RETURN_INVALID_SERVICE_ID : The Service ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI is not initialized
 * ISI_RETURN_FAILED             : Could not convert the IP address specified in address_ptr.
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Service linked list.
 */
ISI_Return ISI_serviceGetFeature(
    ISI_INP ISI_Id serviceId,
    ISI_OUT int  *features_ptr)
{
    ISID_ServiceId *service_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_serviceGet(serviceId, &service_ptr) != ISI_RETURN_FAILED) {
        /*
         * Then we know the id is valid.
         * It may or not be 'activated' but we don't care.
         */
        *features_ptr = service_ptr->features;
        ISID_unlockServices();
        return (ISI_RETURN_OK);
    }
    ISID_unlockServices();
    return (ISI_RETURN_INVALID_SERVICE_ID);
}

/*
 * ======== ISI_setCallResourceStatus() ========
 * This function is used to change the resource status for a call.
 * Once the resource status is set via this function then the resource status
 * can be activated via a call to ISI_modifyCall().
 *
 * callId : The identification number for the call for which the session
 *          direction is to be changed.  This ID number was provided by ISI to
 *          the application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * rsrcStatus : The new resource status for the call. Valid resource status
               are enumerated in the ISI_ResourceStatus.
 *
 * Return Values:
 * ISI_RETURN_OK                  : Session direction changed successfully.
 * ISI_RETURN_INVALID_CALL_ID     : Call ID is invalid.
 * ISI_RETURN_NOT_INIT            : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the linked list.
 */
ISI_Return ISI_setCallResourceStatus(
    ISI_Id               callId,
    ISI_ResourceStatus   rsrcStatus)
{
    ISID_CallId *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid. */
        call_ptr->rsrcStatus |= rsrcStatus;
        ISID_unlockCalls();
        return (ISI_RETURN_OK);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}

/*
 * ======== ISI_getCallResourceStatus() ========
 * This function is used to get resource status of a call.
 *
 * callId : The identification number for the call for which the session
 *          direction is to be changed.  This ID number was provided by ISI to
 *          the application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * rsrcStatus : The new resource status for the call. Valid resource status
 *             are enumerated in the ISI_ResourceStatus.
 *  audioRtpPort: The audio RTP port information of local side.
 *  videoRtpPort: The video RTP port information of local side.
 *  
 * Return Values:
 * ISI_RETURN_OK                  : Session direction changed successfully.
 * ISI_RETURN_INVALID_CALL_ID     : Call ID is invalid.
 * ISI_RETURN_NOT_INIT            : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the linked list.
 */
ISI_Return ISI_getCallResourceStatus(
    ISI_Id               callId,
    ISI_ResourceStatus  *rsrcStatus_ptr,
    int                 *audioRtpPort,
    int                 *videoRtpPort)
{
    ISID_CallId *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid. */
        *rsrcStatus_ptr = call_ptr->rsrcStatus;
        /* Get port status */
        *audioRtpPort = call_ptr->rtpAudioLcl.addr.port;
        *videoRtpPort = call_ptr->rtpVideoLcl.addr.port;

        ISID_unlockCalls();
        return (ISI_RETURN_OK);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}

/*
 * ======== ISI_getCallSrvccStatus() ========
 * This function is used to get SRVCC status of a call.
 *
 * callId : The identification number for the call for which the session
 *          direction is to be changed.  This ID number was provided by ISI to
 *          the application when it initiated a call or delivered to the
 *          application through a ISI event when a call was received.
 *
 * rsrcStatus : The SRVCC status for the call. Valid SRVCC status
               are enumerated in the ISI_SrvccStatus.
 *
 * Return Values:
 * ISI_RETURN_OK                  : Session direction changed successfully.
 * ISI_RETURN_INVALID_CALL_ID     : Call ID is invalid.
 * ISI_RETURN_NOT_INIT            : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the linked list.
 */
ISI_Return ISI_getCallSrvccStatus(
    ISI_Id               callId,
    ISI_SrvccStatus     *srvccStatus_ptr)
{
    ISID_CallId *call_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /* Lock the database of calls */
    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    if (ISID_callGet(callId, &call_ptr) == ISI_RETURN_OK) {
        /* Then we know the id is valid. */
        *srvccStatus_ptr = call_ptr->srvccStatus;
        ISID_unlockCalls();
        return (ISI_RETURN_OK);
    }
    ISID_unlockCalls();
    return (ISI_RETURN_INVALID_CALL_ID);
}

/*
 * ======== ISI_sendUssd ========
 * Applications use this function to send a ussd message to a remote agent.
 *
 * When the message is delivered to the remote agent, ISI will generate a
 * ISI_EVENT_USSD_SEND_OK event signifying that the message was delivered.
 * If the message could not be delivered, ISI will generate a
 * ISI_EVENT_USSD_SEND_FAILED event.
 *
 * msgId_ptr : A pointer to memory location where ISI will write a unique
 *             identification number representing this message transaction.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called
 *
 * type : The type of message being send.
 *
 * to_ptr : A null terminated string containing the address(s) of the remote
 *          agent to send the message to.
 *
 * msg_ptr : A null terminated string containing the ussd body of the message.
 *
 *
 * Return Values:
 * ISI_RETURN_OK              : The message transaction has successfully
 *                              started.
 * ISI_RETURN_FAILED          : Function failed.  The message was not sent.
 * ISI_RETURN_INVALID_CALL_ID : Call ID is invalid.
 * ISI_RETURN_NOT_INIT        : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR     : Could not lock the Ussd Message linked list.
 */

ISI_Return ISI_sendUssd(
    ISI_Id            *msgId_ptr,
    ISI_Id             serviceId,
    ISI_UssdType       type,
    char              *to_ptr,
    char              *msg_ptr)
{
    ISIP_UssdReason  reason;
    ISID_ServiceId  *service_ptr;
    ISID_UssdId     *ussd_ptr;
    ISIP_Ussd       *u_ptr;
    ISI_Return       ret;
    ISI_Id           ussdId;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!msgId_ptr || !to_ptr || !msg_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of Ussds */
    if (ISID_lockUssds() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    ret = ISID_serviceGet(serviceId, &service_ptr);

    if (ret == ISI_RETURN_FAILED) {
        /* Then there is no service */
        ISID_unlockUssds();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }
    else if (ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
        /* Then the service is not active */
        ISID_unlockUssds();
        return (ISI_RETURN_FAILED);
    }
    else {
        /* The service Id is valid and the service is active */
        if ((ussd_ptr = ISID_ussdCreate(service_ptr)) == NULL) {
            /* no available resources */
            ISID_unlockUssds();
            return (ISI_RETURN_FAILED);
        }
        u_ptr = &ussd_ptr->isiMsg.msg.ussd;

        /* Copy the message */
        OSAL_strcpy(u_ptr->message, msg_ptr);
        
        /* Copy the rest */
        ussd_ptr->service_ptr = service_ptr;
        OSAL_strncpy(u_ptr->to, to_ptr, ISI_LONG_ADDRESS_STRING_SZ);
        
        /* Add it to the database.  Make sure msgId_ptr is NULL */
        ussdId = 0;
        if (ISID_ussdAdd(ussd_ptr, &ussdId) != ISI_RETURN_OK) {
            ISI_free(ussd_ptr, ISI_OBJECT_USSD_ID);
            ISID_unlockUssds();
            return (ISI_RETURN_FAILED);
        }
        if (type == ISI_USSD_TYPE_SEND) {
            reason = ISIP_USSD_REASON_SEND;
        }
        else if (type == ISI_USSD_TYPE_REPLY){
            reason = ISIP_USSD_REASON_REPLY;
        }
        else {
            reason = ISIP_USSD_REASON_DISCONNECT; 
        }
        OSAL_logMsg("%s: %d\n", __FUNCTION__, reason);
        /* Populate an ISI Message and send */
        ISIM_sendUssd(reason, ussd_ptr, serviceId);
        ISIT_appUssd(ussd_ptr, &ussd_ptr->isiMsg);
        *msgId_ptr = ussdId;
        ISID_unlockUssds();
        return (ISI_RETURN_OK);
    }
}

/*
 * ======== ISI_readUssd() ========
 * Applications use this function to read a message that arrived from a remote
 * agent. ISI will notify the application of a new incoming message via the
 * ISI_EVENT_USSD_REQUEST event. After receiving this event, applications
 * call this function to retrieve the message from ISI.
 *
 * Application must call this function repeatedly to receive complete message
 * from ISI. Message is completely written when function returns
 * ISI_RETURN_DONE.
 *
 * msgId : An identification number representing the message to retrieve.
 *
 * part_ptr : A pointer to the memory location where ISI will write the message
 *
 * len_ptr : The length of the buffer pointed to by part_ptr. 
 *
 * Return Values:
 * ISI_RETURN_OK                 : Part of the message has been written to part_ptr.
 * ISI_RETURN_FAILED             : Function failed.  Invalid parameters.
 * ISI_RETURN_INVALID_MESSAGE_ID : The message ID is invalid.
 * ISI_RETURN_NOT_INIT           : ISI is not initialized
 * ISI_RETURN_MUTEX_ERROR        : Could not lock the Ussd Message linked list.
 */
ISI_Return ISI_readUssd(
    ISI_Id  msgId,
    char   *part_ptr,
    int    *len_ptr)
{
    ISID_UssdId  *ussd_ptr;
    ISIP_Ussd    *u_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    if (!part_ptr || 0 >= *len_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* Lock the database of Ussds */
    if (ISID_lockUssds() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Look for the message via the messageId */
    if (ISID_ussdGet(msgId, &ussd_ptr) != ISI_RETURN_OK) {
        ISID_unlockUssds();
        return (ISI_RETURN_INVALID_MESSAGE_ID);
    }
    u_ptr = &ussd_ptr->isiMsg.msg.ussd;

       /* Copy the binary data */
       OSAL_strcpy(part_ptr, u_ptr->message);
    
       /* Set length of binary length */
       *len_ptr = strlen(u_ptr->message);

    /* The whole message has been written so free up the ussd resource */
    ISID_ussdDestroy(msgId);
    ISID_unlockUssds();
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_setProvisioningData() ========
 * This function is used to pass a xml doc of paramters configuration to
 * underlying protocol to set the parameters in the xml doc.
 *
 * serviceId : The service identification number returned by ISI when
 *             ISI_allocService() was called
 *
 * xmlDoc_ptr : A null terminated xml doc string containing the provisionoing
 *              data string.  This string represents the provisioning data to
 *              set to the service.
 *
 * Return Values:
 * ISI_RETURN_OK                  : Session direction changed successfully.
 * ISI_RETURN_INVALID_CALL_ID     : Call ID is invalid.
 * ISI_RETURN_NOT_INIT            : ISI not initialized.
 * ISI_RETURN_MUTEX_ERROR         : Could not lock the linked list.
 */
ISI_Return ISI_setProvisioningData(
    ISI_INP ISI_Id      serviceId,
    ISI_INP const char *xmlDoc_ptr)
{
    ISID_ServiceId *service_ptr;
    ISIP_Message   *msg_ptr;

    if (!ISID_getState()) {
        return (ISI_RETURN_NOT_INIT);
    }

    /*
     * Verify all parameters are NOT NULL.
     */
    if (!xmlDoc_ptr) {
        return (ISI_RETURN_FAILED);
    }

    /* take a mutex against the database */
    if (ISID_lockServices() != ISI_RETURN_OK) {
        return (ISI_RETURN_MUTEX_ERROR);
    }

    /* Check the serviceId it may or may not be active.  But we don't care.*/
    if (ISID_serviceGet(serviceId, &service_ptr) == ISI_RETURN_FAILED) {
        /* Then the service does not exist */
        ISID_unlockServices();
        return (ISI_RETURN_INVALID_SERVICE_ID);
    }

    /* Then the id is valid, construct ISIP_Message */
    if (NULL == (msg_ptr = ISIM_setProvisioningData(service_ptr, xmlDoc_ptr))) {
        ISID_unlockServices();
        return (ISI_RETURN_FAILED);
    }

    ISIS_appMsg(msg_ptr);
    ISID_unlockServices();
    return (ISI_RETURN_OK);
}
