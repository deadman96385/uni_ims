/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19918 $ $Date: 2013-02-26 15:29:12 +0800 (Tue, 26 Feb 2013) $
 *
 */

#include <osal.h>
#include <isi.h>
#include <isi_rpc.h>
#include <isi_xdr.h>
#include "_isi_server.h"
#include "_isi_server_rpc.h"
#include <vpad_vpmd.h>

/* Global ISI_ServerObj pointer */
extern ISI_ServerObj *_ISI_ServerObj_ptr;

/*
 * ======== ISI_serverShutdown() ========
 * Public function to shutdown ISI_server.
 *
 * Returns: 
 * OSAL_SUCCESS: ISI_server shutdown successfully 
 * OSAL_FAIL: Error in ISI_server shutdown
 */
OSAL_Status ISI_serverShutdown(
    void)
{
    /* Delete ISI_server task */
    OSAL_taskDelete(_ISI_ServerObj_ptr->taskId);
    OSAL_taskDelete(_ISI_ServerObj_ptr->evtTaskId);

    /* Delete Qs */
    if (NULL != _ISI_ServerObj_ptr->clientEvtQ) {
        OSAL_msgQDelete(_ISI_ServerObj_ptr->clientEvtQ);
    }
    if (NULL != _ISI_ServerObj_ptr->csmEvtQ) {
        OSAL_msgQDelete(_ISI_ServerObj_ptr->csmEvtQ);
    }

    _ISI_serverRpcShutdown(_ISI_ServerObj_ptr);
    
    /* Free _ISI_ServerObj_ptr */
    if (NULL != _ISI_ServerObj_ptr) {
        OSAL_memFree(_ISI_ServerObj_ptr, 0);
        _ISI_ServerObj_ptr = NULL;
    }

    ISI_rpcDbgPrintf("\n");
    return (OSAL_SUCCESS);
}

/* ======== ISI_serverAllocate() ========
 * Public function to initialize ISI_server.
 *
 * Returns: 
 * OSAL_SUCCESS: ISI_server successfully initialized.
 * OSAL_FAIL: Error in ISI_server initialization.
 */
OSAL_Status ISI_serverAllocate(
    void)
{
    if (NULL == (_ISI_ServerObj_ptr = OSAL_memAlloc(sizeof(ISI_ServerObj), 0))) {
        OSAL_logMsg("%s %d: Fail to allocate memory for ISI server object\n",
                 __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    OSAL_memSet(_ISI_ServerObj_ptr, 0, sizeof(ISI_ServerObj));

    /* Create msg Q for ISI event to ISI client */
    if (NULL == (_ISI_ServerObj_ptr->clientEvtQ = OSAL_msgQCreate(
            _ISI_CLIENT_EVT_Q_NAME,
            OSAL_MODULE_ISI_RPC_SERVER, OSAL_MODULE_ISI_RPC_SERVER,
            OSAL_DATA_STRUCT_ISI_ServerEvent,
            _ISI_CLIENT_EVT_Q_NUM,
            sizeof(ISI_ServerEvent), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, _ISI_CLIENT_EVT_Q_NAME);
        return (OSAL_FAIL);
    }

    /* Create msg Q for ISI event to CSM */
    if (NULL == (_ISI_ServerObj_ptr->csmEvtQ = OSAL_msgQCreate(
            _ISI_SERVER_CSM_EVT_Q_NAME,
            OSAL_MODULE_ISI_RPC_SERVER, OSAL_MODULE_ISI_RPC_SERVER,
            OSAL_DATA_STRUCT_ISI_ServerEvent,
            _ISI_SERVER_CSM_EVT_Q_NUM,
            sizeof(ISI_ServerEvent), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, _ISI_SERVER_CSM_EVT_Q_NAME);
        return (OSAL_FAIL);
    }

    if (OSAL_FAIL == _ISI_serverRpcInit(_ISI_ServerObj_ptr)) {
        OSAL_logMsg("%s:%d _ISI_serverRpcInit() failed.\n", __FUNCTION__,
                __LINE__);
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/* ======== ISI_serverStart() ========
 * Public function to create ISI task.
 *
 * Returns: 
 * OSAL_SUCCESS: ISI_server successfully initialized.
 * OSAL_FAIL: Error in ISI_server initialization.
 */
OSAL_Status ISI_serverStart(
    void)
{
    /* Create server main task */
    if (OSAL_SUCCESS != _ISI_startServerTask())
        return (OSAL_FAIL);
    /* Create server event task */
    if (OSAL_SUCCESS != _ISI_startEventTask())
        return (OSAL_FAIL);
    return (OSAL_SUCCESS);
}

/* ======== ISI_serverInit() ========
 * Public function to initialize ISI_server.
 *
 * Returns: 
 * OSAL_SUCCESS: ISI_server successfully initialized.
 * OSAL_FAIL: Error in ISI_server initialization.
 */
OSAL_Status ISI_serverInit(
    void)
{
    if (NULL == (_ISI_ServerObj_ptr = OSAL_memAlloc(sizeof(ISI_ServerObj), 0))) {
        OSAL_logMsg("%s %d: Fail to allocate memory for ISI server object\n",
                 __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    OSAL_memSet(_ISI_ServerObj_ptr, 0, sizeof(ISI_ServerObj));

    /* Create msg Q for ISI event to ISI client */
    if (NULL == (_ISI_ServerObj_ptr->clientEvtQ = OSAL_msgQCreate(
            _ISI_CLIENT_EVT_Q_NAME,
            OSAL_MODULE_ISI_RPC_SERVER, OSAL_MODULE_ISI_RPC_SERVER,
            OSAL_DATA_STRUCT_ISI_ServerEvent,
            _ISI_CLIENT_EVT_Q_NUM,
            sizeof(ISI_ServerEvent), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, _ISI_CLIENT_EVT_Q_NAME);
        return (OSAL_FAIL);
    }

    /* Create msg Q for ISI event to CSM */
    if (NULL == (_ISI_ServerObj_ptr->csmEvtQ = OSAL_msgQCreate(
            _ISI_SERVER_CSM_EVT_Q_NAME,
            OSAL_MODULE_ISI_RPC_SERVER, OSAL_MODULE_ISI_RPC_SERVER,
            OSAL_DATA_STRUCT_ISI_ServerEvent,
            _ISI_SERVER_CSM_EVT_Q_NUM,
            sizeof(ISI_ServerEvent), 0))) {
        OSAL_logMsg("%s:%d Could not create %s queue.\n", __FUNCTION__,
                __LINE__, _ISI_SERVER_CSM_EVT_Q_NAME);
        return (OSAL_FAIL);
    }

    if (OSAL_FAIL == _ISI_serverRpcInit(_ISI_ServerObj_ptr)) {
        OSAL_logMsg("%s:%d _ISI_serverRpcInit() failed.\n", __FUNCTION__,
                __LINE__);
        return (OSAL_FAIL);
    }
    /* Create server main task */
    if (0 == (_ISI_ServerObj_ptr->taskId = OSAL_taskCreate( 
            _ISI_SERVER_TASK_NAME, 
            OSAL_TASK_PRIO_NRT, 
            _ISI_SERVER_TASK_STACK_BYTES, 
            _ISI_serverDaemon, 
            NULL))) { 
        ISI_rpcDbgPrintf("Failed creating %s daemon\n", _ISI_SERVER_TASK_NAME); 
        return (OSAL_FAIL); 
    }

    /* Create server event task */
    if (0 == (_ISI_ServerObj_ptr->evtTaskId = OSAL_taskCreate( 
            _ISI_SERVER_EVT_TASK_NAME, 
            OSAL_TASK_PRIO_NRT, 
            _ISI_SERVER_TASK_STACK_BYTES, 
            _ISI_serverGetEvtTask, 
            0))) { 
        ISI_rpcDbgPrintf("Failed creating %s daemon\n", _ISI_SERVER_TASK_NAME); 
        return (OSAL_FAIL); 
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== ISI_serverGetEvent() ========
 * This function is called by CSM to get ISI event from ISI.
 * And this function examines the retrieved event to see if it's need to
 * returns to CSM or routes to ISI client.
 *
 * Returns: 
 * ISI_RETURN_OK       : An Event was returned.
 * ISI_RETURN_DONE     : Event routed to ISI client, no event returned.
 * ISI_RETURN_FAILED   : There is currently no valid event. This means that
 *                       there was a problem with the underlying select
 *                       statement or with reading an underlying IPC mechanism.
 * ISI_RETURN_TIMEOUT  : The function timed out. There's no event to process.
 * ISI_RETURN_NOT_INIT : ISI not initialized.
 */
ISI_Return ISI_serverGetEvent(
    ISI_Id     *serviceId_ptr,
    ISI_Id     *id_ptr,
    ISI_IdType *idType_ptr,
    ISI_Event  *event_ptr,
    char       *eventDesc_ptr,
    int         timeout)
{
    ISI_Return      ret;
    ISI_ServerEvent isiEvt;
    OSAL_Boolean    sendToIsiClient = OSAL_FALSE;
    vint            size;

    /*
     * Get ISI server event from csmEvtQ for CSM
     * Those events generated by ISI Server for CSM, for example RCS
     * data provisioning.
     */
    if (0 < (size = OSAL_msgQRecv(
            _ISI_ServerObj_ptr->csmEvtQ,
            &isiEvt, sizeof(ISI_ServerEvent), OSAL_NO_WAIT, NULL))) {
        /* Read something. */
        if (size != sizeof(ISI_ServerEvent)) {
            OSAL_logMsg("%s %d: Incorrect msg size received. size:%d\n",
                    __FUNCTION__, __LINE__, size);
        }
        else {
            /* Got an event */
            *serviceId_ptr = isiEvt.serviceId;
            *id_ptr = isiEvt.id;
            *idType_ptr = isiEvt.type;
            *event_ptr = isiEvt.evt;
            OSAL_memCpy(eventDesc_ptr, isiEvt.eventDesc,
                    ISI_EVENT_DESC_STRING_SZ);
            return (ISI_RETURN_OK);
        }
    }

    ret = ISI_getEvent(serviceId_ptr, id_ptr, idType_ptr, event_ptr,
            eventDesc_ptr, timeout);
    if (ret == ISI_RETURN_OK) {
        /* Process event */
        if (_ISI_serverIsEventForBoth(*idType_ptr, *event_ptr)) {
            sendToIsiClient = OSAL_TRUE;
            /* Don't change return value so it get's processed locally also. */
        }
        else if (_ISI_serverIsEventForIsiClient(serviceId_ptr, *id_ptr,
                    *idType_ptr, *event_ptr, eventDesc_ptr)) {
            sendToIsiClient = OSAL_TRUE;
            /* Return ISI_RETURN_DONE to indicate nothing get */
            ret = ISI_RETURN_DONE;
        }

        if (sendToIsiClient) {
            /* Send to ISI client event queue */
            isiEvt.serviceId = *serviceId_ptr;
            isiEvt.id = *id_ptr;
            isiEvt.type = *idType_ptr;
            isiEvt.evt = *event_ptr;
            OSAL_memCpy(isiEvt.eventDesc, eventDesc_ptr,
                    ISI_EVENT_DESC_STRING_SZ);
            if (OSAL_SUCCESS != OSAL_msgQSend(_ISI_ServerObj_ptr->clientEvtQ,
                    &isiEvt, sizeof(ISI_ServerEvent), OSAL_NO_WAIT, NULL)) {
                ISI_rpcDbgPrintf("Failed to write ISI event to client evt Q.\n");
            }
        }
    }

    /* Return what we get from ISI */
    return (ret);
}

/*
 * ======== ISI_serverGetProvisioningData() ========
 * This function is called by CSM to get RCS provisioning data which is set
 * from ISI Client by calling ISI_serviceSetProvisioningData().
 *
 * Returns: 
 * ISI_RETURN_OK       : An Event was returned.
 * ISI_RETURN_DONE     : Event routed to ISI client, no event returned.
 * ISI_RETURN_FAILED   : There is currently no valid event. This means that
 *                       there was a problem with the underlying select
 *                       statement or with reading an underlying IPC mechanism.
 * ISI_RETURN_TIMEOUT  : The function timed out. There's no event to process.
 * ISI_RETURN_NOT_INIT : ISI not initialized.
 */
ISI_Return ISI_serverGetProvisioningData(
    ISI_Id serviceId,
    char   *xmlDoc_ptr,
    char   *uri_ptr,
    char   *username_ptr,
    char   *password_ptr,
    char   *realm_ptr,
    char   *proxy_ptr,
    char   *obProxy_ptr,
    char   *imConfUri_ptr)
{
    /*
     * serviceId is not used currently, we now only support RCS provisioning on
     * single service.
     */
    OSAL_strncpy(xmlDoc_ptr, _ISI_ServerObj_ptr->service.provisioningData,
            ISI_PROVISIONING_DATA_STRING_SZ);

    /* Also get all the service configurations */
    if (0 != _ISI_ServerObj_ptr->service.szUri[0]) {
        OSAL_strncpy(uri_ptr, _ISI_ServerObj_ptr->service.szUri,
                sizeof(_ISI_ServerObj_ptr->service.szUri));
    }
    if (0 != _ISI_ServerObj_ptr->service.szUsername[0]) {
        OSAL_strncpy(username_ptr, _ISI_ServerObj_ptr->service.szUsername,
                sizeof(_ISI_ServerObj_ptr->service.szUsername));
    }
    if (0 != _ISI_ServerObj_ptr->service.szPassword[0]) {
        OSAL_strncpy(password_ptr, _ISI_ServerObj_ptr->service.szPassword,
                sizeof(_ISI_ServerObj_ptr->service.szPassword));
    }
    if (0 != _ISI_ServerObj_ptr->service.szRealm[0]) {
        OSAL_strncpy(realm_ptr, _ISI_ServerObj_ptr->service.szRealm,
                sizeof(_ISI_ServerObj_ptr->service.szRealm));
        /* Also set the realm to domain */
        OSAL_strncpy(_ISI_ServerObj_ptr->service.szDomain,
                _ISI_ServerObj_ptr->service.szRealm,
                sizeof(_ISI_ServerObj_ptr->service.szDomain));
    }
    if (0 != _ISI_ServerObj_ptr->service.szProxy[0]) {
        OSAL_strncpy(proxy_ptr, _ISI_ServerObj_ptr->service.szProxy,
                sizeof(_ISI_ServerObj_ptr->service.szProxy));
    }
    if (0 != _ISI_ServerObj_ptr->service.szOutboundProxy[0]) {
        OSAL_strncpy(obProxy_ptr, _ISI_ServerObj_ptr->service.szOutboundProxy,
                sizeof(_ISI_ServerObj_ptr->service.szOutboundProxy));
    }
    if (0 != _ISI_ServerObj_ptr->service.imConfUri[0]) {
        OSAL_strncpy(imConfUri_ptr, _ISI_ServerObj_ptr->service.imConfUri,
                sizeof(_ISI_ServerObj_ptr->service.imConfUri));
    }
    return (ISI_RETURN_OK);
}    

/*
 * ======== ISI_serverSetNetworkMode() ========
 * This function is used to configure what network mode, LTE or WiFi.
 *
 * Returns: 
 * ISI_RETURN_OK       : successful to configure.
 * ISI_RETURN_FAILED   : fail to configure
 */
ISI_Return ISI_serverSetNetworkMode(
    ISI_NetworksMode mode)
{
    _ISI_ServerObj_ptr->networkMode = mode;
    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_serverSetNetworkMode() ========
 * This function is used set the domain to ISI Server for normalizing
 * outbound remote address.
 *
 * domain_ptr: Null terminated string contains the domain
 * Returns: 
 * ISI_RETURN_OK       : successful to configure.
 * ISI_RETURN_FAILED   : fail to configure
 */
ISI_Return ISI_serverSetDomain(
    const char *domain_ptr)
{
    OSAL_strncpy(_ISI_ServerObj_ptr->service.szDomain, domain_ptr, 
            ISI_ADDRESS_STRING_SZ);

    return (ISI_RETURN_OK);
}

/*
 * ======== ISI_serverSetMediaSessionType() ========
 * This function is used to configure the media session type.
 * <parm name="psMedia" value="MSRPoTLS"/>
 * <parm name="wifiMedia" value="MSRP"/>
 *
 * Returns: 
 * ISI_RETURN_OK       : successful to configure.
 * ISI_RETURN_FAILED   : fail to configure
 */
ISI_Return ISI_serverSetMediaSessionType(
    ISI_NetworksMode    mode,
    ISI_SessionType     type)
{
    if (ISI_NETWORK_MODE_LTE == mode) {
        if (ISI_SESSION_TYPE_CHAT == type) {
            _ISI_ServerObj_ptr->psMediaSessionType |= ISI_SESSION_TYPE_CHAT;
            return (ISI_RETURN_OK);
        }
        else if (ISI_SESSION_TYPE_SECURITY_CHAT == type){
            _ISI_ServerObj_ptr->psMediaSessionType |=
                    (ISI_SESSION_TYPE_CHAT | ISI_SESSION_TYPE_SECURITY_CHAT);
            return (ISI_RETURN_OK);
        }
    }
    else if (ISI_NETWORK_MODE_WIFI == mode) {
        if (ISI_SESSION_TYPE_CHAT == type) {
            _ISI_ServerObj_ptr->wifiMediaSessionType |= ISI_SESSION_TYPE_CHAT;
            return (ISI_RETURN_OK);
        }
        else if (ISI_SESSION_TYPE_SECURITY_CHAT == type){
            _ISI_ServerObj_ptr->wifiMediaSessionType |=
                    (ISI_SESSION_TYPE_CHAT | ISI_SESSION_TYPE_SECURITY_CHAT);
            return (ISI_RETURN_OK);
        }
    }

    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISI_serverSetRTMediaSessionType() ========
 * This function is used to configure thereal time media session type.
 * <parm name="psRTMedia" value="RTP"/>
 * <parm name="wifiRTMedia" value="SRTP"/>
 *
 * Returns: 
 * ISI_RETURN_OK       : successful to configure.
 * ISI_RETURN_FAILED   : fail to configure
 */
ISI_Return ISI_serverSetRTMediaSessionType(
    ISI_NetworksMode    mode,
    ISI_RTMedia     type)
{
    if (ISI_NETWORK_MODE_LTE == mode) {
        _ISI_ServerObj_ptr->psRTMediaSessionType = type;
        return (ISI_RETURN_OK);
    }      
    else if (ISI_NETWORK_MODE_WIFI == mode) {
        _ISI_ServerObj_ptr->wifiRTMediaSessionType = type;
        return (ISI_RETURN_OK);
    }
    return (ISI_RETURN_FAILED);
}

/*
 * ======== ISI_serverSetConfCallId() ========
 * This function is used to set  Conference call id to isi_server
 * Returns: 
 * ISI_RETURN_OK       : successful to configure.
 * ISI_RETURN_FAILED   : fail to configure
 */
ISI_Return ISI_serverSetConfCallId(
    ISI_Id  callId)
{
    _ISI_ServerObj_ptr->confCallId = callId;
    return (ISI_RETURN_OK);
}
