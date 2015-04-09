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
#include <isip.h>
#include <isi_rpc.h>
#include <isi_xdr.h>
#include "_isi_server.h"
#include "_isi_server_rpc.h"
#include <vpad_vpmd.h>
#include <rpm.h>

/* Global ISI_ServerObj pointer */
ISI_ServerObj *_ISI_ServerObj_ptr = NULL;

/*
 * ======== _ISI_serverDbEntryFind() ========
 *
 * Private function to look for if there is target id in database.
 * 
 * Returns: 
 * OSAL_TRUE      : There is target id in DB.
 * OSAL_FALSE     : There is no target id in DB.
 */
OSAL_Boolean _ISI_serverDbEntryFind(
    ISI_IdType  type,
    ISI_Id      id)
{
    vint idx;

    for (idx = 0; idx < _ISI_SERVER_DB_MAX_ENTRIES; idx++) {
        if ((type == _ISI_ServerObj_ptr->db[idx].type) &&
                (id == _ISI_ServerObj_ptr->db[idx].id)) {
            return (OSAL_TRUE);
        }
    }
    return (OSAL_FALSE);
}

/*
 * ======== _ISI_serverDbEntryAdd() ========
 *
 * Private function to add target id into DB.
 * 
 * Returns: 
 * OSAL_SUCCESS  : success to add it.
 * OSAL_FAIL     : fail to add it.
 */
OSAL_Status _ISI_serverDbEntryAdd(
    ISI_IdType  type,
    ISI_Id      id)
{
    vint idx;

    /* If it is already in database, return SUCCESS. */
    if (OSAL_TRUE == (_ISI_serverDbEntryFind(type, id))) {
        return (OSAL_SUCCESS);
    }

    for (idx = 0; idx < _ISI_SERVER_DB_MAX_ENTRIES; idx++) {
        if ((ISI_ID_TYPE_NONE == _ISI_ServerObj_ptr->db[idx].type) &&
                (0 == _ISI_ServerObj_ptr->db[idx].id)) {
            _ISI_ServerObj_ptr->db[idx].type = type;
            _ISI_ServerObj_ptr->db[idx].id   = id;
            return (OSAL_SUCCESS);
        }
    }
    return (OSAL_FAIL);
}

/*
 * ======== _ISI_serverDbEntryDestroy() ========
 *
 * Private function to remove target id from DB.
 * 
 * Returns: 
 * OSAL_SUCCESS  : success to remove it.
 * OSAL_FAIL     : fail to remove it.
 */
OSAL_Status _ISI_serverDbEntryDestroy(
    ISI_IdType  type,
    ISI_Id      id)
{
    vint idx;

    for (idx = 0; idx < _ISI_SERVER_DB_MAX_ENTRIES; idx++) {
        if ((type == _ISI_ServerObj_ptr->db[idx].type) &&
                (id == _ISI_ServerObj_ptr->db[idx].id)) {
            _ISI_ServerObj_ptr->db[idx].type = ISI_ID_TYPE_NONE;
            _ISI_ServerObj_ptr->db[idx].id   = 0;
            return (OSAL_SUCCESS);
        }
    }
    return (OSAL_FAIL);
}

/*
 * ======== _ISI_serverGetServiceByFeature() ========
 *
 * Private function to get service by feature support.
 * 
 * Returns: 
 * OSAL_SUCCESS  : Found service.
 * OSAL_FAIL     : Service wasn't found.
 */
OSAL_Status _ISI_serverGetServiceByFeature(
    ISI_Id          *serviceId_ptr,
    ISI_FeatureType  feature)
{
    vint    idx;
    ISI_Id  serviceId;
    int     protocol;
    int     isEmergency;
    int     features;
    int     isActivated;

    serviceId = 0;
    for (idx = 0; idx < _ISI_SERVER_MAX_SERVICES; idx++) {
        if (ISI_RETURN_OK == ISI_getNextService(&serviceId, &protocol,
                &isEmergency, &features, &isActivated)) {
            if (features & feature) {
                /* This service support the feature */
                *serviceId_ptr = serviceId;
                return (OSAL_SUCCESS);
            }
        }
        else {
            /* No more service */
            break;
        }
    }

    ISI_rpcDbgPrintf("Cannot find service. Feature:%d\n", feature);
    return (OSAL_FAIL);
}

/*
 * ======== _ISI_serverUpdateServiceId() ========
 *
 * Private function to update service id for ISI_Event send to csi.
 * Csi manage a specific service id, currently it's the service id of RCS SIP.
 * All isi event for csi will be changed to the specific service id so that csi
 * would process it.
 *
 * Returns: 
 * OSAL_SUCCESS  : Found service.
 * OSAL_FAIL     : Service wasn't found.
 */
OSAL_Status _ISI_serverUpdateServiceId(
    ISI_Id          *serviceId_ptr)
{
    vint    idx;
    ISI_Id  serviceId;
    int     protocol;
    int     isEmergency;
    int     features;
    int     isActivated;

    serviceId = 0;
    for (idx = 0; idx < _ISI_SERVER_MAX_SERVICES; idx++) {
        if (ISI_RETURN_OK == ISI_getNextService(&serviceId, &protocol,
                &isEmergency, &features, &isActivated)) {
            if (ISI_PROTOCOL_SIP_RCS == protocol) {
                /* Got the service */
                *serviceId_ptr = serviceId;
                return (OSAL_SUCCESS);
            }
        }
        else {
            /* No more service */
            break;
        }
    }

    ISI_rpcDbgPrintf("Cannot find service. protocol:%d\n", ISI_PROTOCOL_SIP_RCS);
    return (OSAL_FAIL);
}

/*
 * ======== _ISI_serverTerminateAllCalls() ========
 *
 * Private function terminated all calls those associated to ISI Client.
 * 
 * Returns: 
 *  OSAL_SUCCESS: Terminated successully.
 *  OSAL_FAIL: Failed for some reasons.
 */
OSAL_Status _ISI_serverTerminateAllCalls(
    void)
{
    vint idx;
    vint id;

    for (idx = 0; idx < _ISI_SERVER_DB_MAX_ENTRIES; idx++) {
        if (ISI_ID_TYPE_CALL == _ISI_ServerObj_ptr->db[idx].type) {
            id = _ISI_ServerObj_ptr->db[idx].id;
        ISI_rpcDbgPrintf("Terminating zombie call. Id:%d\n", id);
            ISI_terminateCall(id, NULL);
            _ISI_serverDbEntryDestroy(ISI_ID_TYPE_CALL, id);
        }
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _ISI_serverIsEventForBoth() ========
 *
 * Private function to check if the ISI event is for
 * both the ISI client and locally.
 *
 * Returns:
 * OSAL_TRUE      : The event is for ISI client and local processing.
 * OSAL_FALSE     : The event is not for ISI client and local Processing.
 */
OSAL_Boolean _ISI_serverIsEventForBoth(
    ISI_IdType   type,
    ISI_Event    evt)
{
    /*
     * The ISI Client should get events regarding
     * protocol availability and service state
     */
    switch (type) {
        case ISI_ID_TYPE_NONE:
            /* ISI Client wants event s to know things are alive or dead. */
            if (ISI_EVENT_PROTOCOL_READY == evt) {
                return (OSAL_TRUE);
            }
            if (ISI_EVENT_PROTOCOL_FAILED == evt) {
                /*
                 * Protocol failed. Assume mCUE restarted.
                 * Terminate all zombie calls.
                 */
                _ISI_serverTerminateAllCalls();
                return (OSAL_TRUE);
            }
            break;
        case ISI_ID_TYPE_SERVICE:
            switch (evt) {
                case ISI_EVENT_SERVICE_ACTIVE:
                case ISI_EVENT_CREDENTIALS_REJECTED:
                case ISI_EVENT_NET_UNAVAILABLE:
                case ISI_EVENT_SERVICE_INACTIVE:
                case ISI_EVENT_SERVICE_HANDOFF:
                case ISI_EVENT_AKA_AUTH_REQUIRED:
                case ISI_EVENT_IPSEC_SETUP:
                case ISI_EVENT_IPSEC_RELEASE:
                    return (OSAL_TRUE);
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return (OSAL_FALSE);
}

/*
 * ======== _ISI_serverIsEventForIsiClient() ========
 *
 * Private function to check if the ISI evnet is for ISI client or not.
 * 
 * Returns: 
 * OSAL_TRUE      : The event is for ISI client.
 * OSAL_FALSE     : The event is not for ISI client.
 */
OSAL_Boolean _ISI_serverIsEventForIsiClient(
    ISI_Id      *serviceId_ptr,
    ISI_Id       id,
    ISI_IdType   type,
    ISI_Event    evt,
    char        *eventDesc_ptr)
{
    uint16          sessType;
    ISI_FeatureType features;

    features = _ISI_ServerObj_ptr->features;
    /*
     * 1. If AT interface is enabled. If not, route all call and SMS to
     *    ISI client.
     * 2. If an ISI event is any response to the command from ISI client
     */
    switch (type) {
        case ISI_ID_TYPE_CALL:
            /* It's a call event, look into database first */
            if (OSAL_TRUE == _ISI_serverDbEntryFind(type, id)) {
                /*
                 * It's an event for ISI client, see if it's a call end
                 * event, if yes then remove it from database.
                 */
                if ((ISI_EVENT_CALL_REJECTED == evt) ||
                        (ISI_EVENT_CALL_FAILED == evt) ||
                        (ISI_EVENT_NET_UNAVAILABLE == evt) ||
                        (ISI_EVENT_CALL_DISCONNECTED == evt)) {
                    _ISI_serverDbEntryDestroy(type, id);
                }
                /* Update service id for ISI client */
                _ISI_serverUpdateServiceId(serviceId_ptr);
                return (OSAL_TRUE);
            }
            /* Then check by features set */
            if (ISI_RETURN_OK == ISI_getCallSessionType(id, &sessType)) {
                if ((sessType & ISI_SESSION_TYPE_VIDEO) ||
                        (sessType & ISI_SESSION_TYPE_SECURITY_VIDEO)) {
                    /* It's a video call */
                    if (features & ISI_FEATURE_TYPE_RCS) {
                        _ISI_serverDbEntryAdd(type, id);
                        /* Update service id for ISI client */
                        _ISI_serverUpdateServiceId(serviceId_ptr);
                        return (OSAL_TRUE);
                    }
                }
                if (sessType & ISI_SESSION_TYPE_CHAT) {
                    /* Chat session */
                    if (features & ISI_FEATURE_TYPE_RCS) {
                        _ISI_serverDbEntryAdd(type, id);
                        /* Update service id for ISI client */
                        _ISI_serverUpdateServiceId(serviceId_ptr);
                        return (OSAL_TRUE);
                    }
                }
                else {
                    /* Must be audio only call */
                    if (features & ISI_FEATURE_TYPE_VOLTE_CALL) {
                        _ISI_serverDbEntryAdd(type, id);
                        /* Update service id for ISI client */
                        _ISI_serverUpdateServiceId(serviceId_ptr);
                        return (OSAL_TRUE);
                    }
                }
            }
            else {
                /* Fail to get session type */
                return (OSAL_FALSE);
            }
            break;
        case ISI_ID_TYPE_MESSAGE:
            /* It's a message event, look into database first */
            if (OSAL_TRUE == _ISI_serverDbEntryFind(type, id)) {
                /*
                 * It's an event for ISI client, see if it's a message
                 * complete event, if yes then remove it from database.
                 */
                if ((ISI_EVENT_MESSAGE_SEND_OK == evt) ||
                        (ISI_EVENT_MESSAGE_SEND_FAILED == evt)) {
                    _ISI_serverDbEntryDestroy(type, id);
                }
                /* Update service id for ISI client */
                _ISI_serverUpdateServiceId(serviceId_ptr);
                return (OSAL_TRUE);
            }
            /* Then check by features set */
            if (features & ISI_FEATURE_TYPE_VOLTE_SMS) {
                _ISI_serverDbEntryAdd(type, id);
                /* Update service id for ISI client */
                _ISI_serverUpdateServiceId(serviceId_ptr);
                return (OSAL_TRUE);
            }
            break;
        case ISI_ID_TYPE_CHAT:
        case ISI_ID_TYPE_FILE:
            if (features & ISI_FEATURE_TYPE_RCS) {
                /* Update service id for ISI client */
                _ISI_serverUpdateServiceId(serviceId_ptr);
                return (OSAL_TRUE);
            }
            break;
         case ISI_ID_TYPE_PRESENCE:
            /* if the call id is conference call id that means the event is for csm */
            if (evt == ISI_EVENT_GROUP_CHAT_PRES_RECEIVED) {
                if (NULL != eventDesc_ptr) {
                    if (OSAL_strscan(eventDesc_ptr, "ChatId")) {
                        if (_ISI_ServerObj_ptr->confCallId == 
                                (ISI_Id)OSAL_atoi(eventDesc_ptr + 7)) {
                        ISI_rpcDbgPrintf("%s %d Chat Id matched",
                                    __FILE__, __LINE__);
                            return (OSAL_FALSE);
                        }
                    }
                }
            }
            if (features & ISI_FEATURE_TYPE_RCS) {
                /* Update service id for ISI client */
                _ISI_serverUpdateServiceId(serviceId_ptr);
                return (OSAL_TRUE);
            }
            break;
        default:
            break;
    }

    return (OSAL_FALSE);
}

/*
 * ======== _ISI_serverInit() ========
 * This function is used to fo ISI init.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverInit(
    ISI_Xdr   *xdr_ptr)
{
    /* Set default features, RCS features route to ISI client */
    _ISI_ServerObj_ptr->features = ISI_FEATURE_TYPE_RCS;

    ISI_xdrEncodeInit(xdr_ptr);
    /* Always return ok to client */
    ISI_xdrPutInteger(xdr_ptr, ISI_RETURN_OK);
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }   
}

/*
 * ======== _ISI_serverSetFeature() ========
 * This function is to set features to ISI server so that the events relates
 * to features will be directed to ISI client rather than CSM.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSetFeature(
    ISI_Xdr   *xdr_ptr)
{
    /*
     * Prototype:
     * ISI_Return ISI_setFeature(
     *  ISI_INP ISI_FeatureType features);
     */

    _ISI_SERVER_VERIFY_INIT;

    /* Get features and store in server object */
    ISI_xdrGetInteger(xdr_ptr, (int *)&_ISI_ServerObj_ptr->features);

    /* Debug information */
    if (_ISI_ServerObj_ptr->features & ISI_FEATURE_TYPE_VOLTE_CALL) {
        ISI_rpcDbgPrintf("ISI_FEATURE_TYPE_VOLTE_CALL set.\n");
    }
    if (_ISI_ServerObj_ptr->features & ISI_FEATURE_TYPE_VOLTE_SMS) {
        ISI_rpcDbgPrintf("ISI_FEATURE_TYPE_VOLTE_SMS set.\n");
    }
    if (_ISI_ServerObj_ptr->features & ISI_FEATURE_TYPE_RCS) {
        ISI_rpcDbgPrintf("ISI_FEATURE_TYPE_VOLTE_RCS set.\n");
    }

    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response, always return ok */
    ISI_xdrPutInteger(xdr_ptr, ISI_RETURN_OK);
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data,
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }

    return;
}

/*
 * ======== _ISI_serverGetEvent() ========
 *
 * ISI server mapping function to ISI_getEvent().
 * This function recevies msg from ISI server event queue to see if there
 * is any event for ISI client.
 *
 * Returns: 
 * None.
 */
void _ISI_serverGetEvent(
    ISI_Xdr   *xdr_ptr)
{
    /*
     * Prototype:
     * ISI_Return ISI_getEvent(
     *  ISI_OUT ISI_Id     *serviceId_ptr,
     *  ISI_OUT ISI_Id     *id_ptr,
     *  ISI_OUT ISI_IdType *idType_ptr,
     *  ISI_OUT ISI_Event  *event_ptr,
     *  ISI_OUT char       *eventDesc_ptr,
     *  ISI_INP int         timeout);
     */
    int             timeout;
    ISI_ServerEvent serverEvt;

    _ISI_SERVER_VERIFY_INIT;

    /* Get timeout */
    ISI_xdrGetInteger(xdr_ptr, (int *)&timeout);

    /* Receive queue */
    if (sizeof(ISI_ServerEvent) != OSAL_msgQRecv(
            _ISI_ServerObj_ptr->clientEvtQ,
            &serverEvt, sizeof(ISI_ServerEvent), timeout, NULL)) {
        OSAL_logMsg("%s %d: Incorrect msg size received.\n", __FUNCTION__,
                __LINE__);
        return;
    }

    /* We got the event, send back */
    ISI_xdrEncodeInit(xdr_ptr);
    /* Put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ISI_RETURN_OK);
    /* Put *serviceId_ptr */
    ISI_xdrPutInteger(xdr_ptr, serverEvt.serviceId);
    /* Put id */
    ISI_xdrPutInteger(xdr_ptr, serverEvt.id);
    /* Put type */
    ISI_xdrPutInteger(xdr_ptr, serverEvt.type);
    /* Put event */
    ISI_xdrPutInteger(xdr_ptr, serverEvt.evt);
    /* Put eventDesc */
    ISI_xdrPutString(xdr_ptr, serverEvt.eventDesc,
            OSAL_strlen(serverEvt.eventDesc) + 1);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsiEvt(
            _ISI_ServerObj_ptr, xdr_ptr->data,
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
    return;
}

/*
 * ======== _ISI_serverInitiateCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverInitiateCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return            ret;
    ISI_Id                callId;
    ISI_Id                serviceId;
    char                  to[ISI_ADDRESS_STRING_SZ + 1];
    int                   to_length;
    char                  subject[ISI_SUBJECT_STRING_SZ + 1];
    int                   subject_length;                    
    ISI_SessionCidType    cidType;
    char                  mediaAttribute[ISI_MEDIA_ATTR_STRING_SZ + 1];
    int                   mediaAttribute_length;
    char                  normalizedAddress[ISI_ADDRESS_STRING_SZ + 1];

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);

    /* Get appropriate service Id for making call */
    _ISI_serverGetServiceByFeature(&serviceId, ISI_FEATURE_TYPE_VOLTE_CALL);

    /* Get to uri */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)to, &to_length)) {
        return;
    }

    /* Normalize the remote address */
    RPM_normalizeOutboundAddress(_ISI_ServerObj_ptr->service.szDomain, to,
            normalizedAddress, ISI_ADDRESS_STRING_SZ,
            RPM_FEATURE_TYPE_CALL_NORMAL);

    /* Get subject */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)subject,
            &subject_length)) {
        return;
    }
#if 0    
    /* Get current network mode and set the session type */
    if (ISI_NETWORK_MODE_LTE == _ISI_ServerObj_ptr->networkMode &&
            ISI_TRASPORT_PROTO_RT_MEDIA_SRTP == 
            _ISI_ServerObj_ptr->psRTMediaSessionType) {
        if (ISI_SESSION_TYPE_AUDIO & callType) {
            callType |= ISI_SESSION_TYPE_SECURITY_AUDIO;
        }
        if (ISI_SESSION_TYPE_VIDEO & callType) {
            callType |= ISI_SESSION_TYPE_SECURITY_VIDEO;
        }
    }
    else if (ISI_NETWORK_MODE_WIFI == _ISI_ServerObj_ptr->networkMode &&
            ISI_TRASPORT_PROTO_RT_MEDIA_SRTP == 
            _ISI_ServerObj_ptr->wifiRTMediaSessionType) {
        if (ISI_SESSION_TYPE_AUDIO & callType) {
            callType |= ISI_SESSION_TYPE_SECURITY_AUDIO;
        }
        if (ISI_SESSION_TYPE_VIDEO & callType) {
            callType |= ISI_SESSION_TYPE_SECURITY_VIDEO;
        }
    }
#endif    
    /* Get cidType */
    ISI_xdrGetInteger(xdr_ptr, (int*)&cidType);
    /* Get mediaAttribute */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)mediaAttribute,
            &mediaAttribute_length)) {
        return;
    }

    ret = ISI_initiateCall(&callId, serviceId, normalizedAddress, subject,
            cidType, mediaAttribute);
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put call Id */
    ISI_xdrPutInteger(xdr_ptr, callId);
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }

    /* to put the callId into the database for tracing it */
    if (ISI_RETURN_OK == ret) {
        _ISI_serverDbEntryAdd(ISI_ID_TYPE_CALL, callId);
    }
    return;
}

/*
 * ======== _ISI_serverInitiateCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverUpdateCallSession(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return        ret;
    ISI_Id            callId;
    char              mediaAttribute[ISI_MEDIA_ATTR_STRING_SZ + 1];
    int               mediaAttribute_length;

    /* Get call Id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);

    /* Get mediaAttribute */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)mediaAttribute,
            &mediaAttribute_length)) {
        return;
    }
    ret = ISI_updateCallSession(callId, mediaAttribute);

    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetNextService() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetNextService(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return ret;
    ISI_Id     serviceId;
    int        protocol;
    int        isEmergency;
    int        features;
    int        isActivated;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);

    ret = ISI_getNextService(&serviceId, &protocol, &isEmergency, &features,
            &isActivated);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put ISI return serviceId */
    ISI_xdrPutInteger(xdr_ptr, serviceId);
    /* put ISI return protocol */
    ISI_xdrPutInteger(xdr_ptr, protocol);
    /* put ISI return isEmergency */
    ISI_xdrPutInteger(xdr_ptr, isEmergency);
    /* put ISI return features*/
    ISI_xdrPutInteger(xdr_ptr, features);
    /* put ISI return isActivated */
    ISI_xdrPutInteger(xdr_ptr, isActivated);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSetFeature() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceSetFeature(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    int             features;

    /*
     * ISI API prototype:
     * ISI_Return ISI_serviceSetFeature(
     * ISI_INP ISI_Id serviceId,
     * ISI_INP int    features);
     */

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get features */
    ISI_xdrGetInteger(xdr_ptr, &features);

    ret = ISI_serviceSetFeature(serviceId, features);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetFeature() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceGetFeature(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    int             features;

    /* 
     * ISI API prototype:
     * ISI_Return ISI_serviceGetFeature(
     * ISI_INP ISI_Id serviceId,
     * ISI_OUT int   *features_ptr);
     */

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);

    ret = ISI_serviceGetFeature(serviceId, &features);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put ISI return features*/
    ISI_xdrPutInteger(xdr_ptr, features);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSetCallResourceStatus() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSetCallResourceStatus(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          callId;
    int             rsrcStatus;

    /* Get call id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);

    /* Get rsrcStatus id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&rsrcStatus);

    ret = ISI_setCallResourceStatus(callId, rsrcStatus);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetCallResourceStatus() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetCallResourceStatus(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             callId;
    ISI_ResourceStatus rsrcStatus;
    int     audioRtpPort;
    int     videoRtpPort;

    /* Get call id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);

    ret = ISI_getCallResourceStatus(callId, &rsrcStatus,
            &audioRtpPort, &videoRtpPort);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put ISI rsrcStatus */
    ISI_xdrPutInteger(xdr_ptr, rsrcStatus);
    /* put ISI audioRtpPort */
    ISI_xdrPutInteger(xdr_ptr, audioRtpPort);
    /* put ISI videoRtpPort */
    ISI_xdrPutInteger(xdr_ptr, videoRtpPort);
    

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetCallSrvccStatus() ========
 * This function is used to map the ISI functions.
 *
 * Return Values:
 *   None
 */
void _ISI_serverGetCallSrvccStatus(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             callId;
    ISI_SrvccStatus    srvccStatus;

    /* Get call id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);

    ret = ISI_getCallSrvccStatus(callId, &srvccStatus);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put ISI rsrcStatus */
    ISI_xdrPutInteger(xdr_ptr, srvccStatus);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data,
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetVersion() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetVersion(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    char            version[ISI_VERSION_STRING_SZ + 1];

    ret = ISI_getVersion(version);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put version */
    ISI_xdrPutString(xdr_ptr, version, OSAL_strlen(version) + 1);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverShutdown() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverShutdown(
    ISI_Xdr   *xdr_ptr)
{
    /* Currently always return ok to client */

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ISI_RETURN_OK);
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAllocService() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAllocService(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    int             protocol;

    /* Get protocol */
    ISI_xdrGetInteger(xdr_ptr, (int*)&protocol);

    ret = ISI_allocService(&serviceId, protocol);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put serviceId */
    ISI_xdrPutInteger(xdr_ptr, serviceId);
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverActivateService() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverActivateService(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    ret = ISI_activateService(serviceId);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverDeactivateService() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverDeactivateService(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    ret = ISI_deactivateService(serviceId);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverFreeService() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverFreeService(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    ret = ISI_freeService(serviceId);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceMakeCidPrivate() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceMakeCidPrivate(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    int             isPrivate;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get isPrivate */
    ISI_xdrGetInteger(xdr_ptr, (int*)&isPrivate);
    
    ret = ISI_serviceMakeCidPrivate(serviceId, isPrivate);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceSetBsid() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceSetBsid(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return            ret;
    ISI_Id                serviceId;
    ISI_NetworkAccessType type;
    char                  bsid[ISI_BSID_STRING_SZ + 1];
    int                   bsidLength;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get type */
    ISI_xdrGetInteger(xdr_ptr, (int*)&type);
    /* Get bsid */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)bsid, &bsidLength)) {
        return;
    }
   
    ret = ISI_serviceSetBsid(serviceId, type, bsid);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceSetBsid() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceSetImeiUri(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    char            imei[ISI_ADDRESS_STRING_SZ + 1];
    int             imei_length;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get imei */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)imei, &imei_length)) {
        return;
    }
   
    ret = ISI_serviceSetImeiUri(serviceId, imei);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceSetInterface() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceSetInterface(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    char            name[ISI_ADDRESS_STRING_SZ + 1];
    int             name_length;
    char            address[ISI_ADDRESS_STRING_SZ + 1];
    int             address_length;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get name */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)name, &name_length)) {
        return;
    }
    /* Get name */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)address,
            &address_length)) {
        return;
    }
   
    ret = ISI_serviceSetInterface(serviceId, name, address);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceSetUri() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceSetUri(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    int             uri_length;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get uri and store it to service struct in _ISI_ServerObj_ptr */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr,
            (char *)_ISI_ServerObj_ptr->service.szUri, &uri_length)) {
        _ISI_ServerObj_ptr->service.szUri[0] = 0;
        return;
    }
    
    /* No need to call ISI api, just return OK. */
    ret = ISI_RETURN_OK;
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceSetCapabilities() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceSetCapabilities(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    char            capabilities[ISI_PRESENCE_STRING_SZ + 1];
    int             capab_length;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get capabilities */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)capabilities,
            &capab_length)) {
        return;
    }

    ret = ISI_serviceSetCapabilities(serviceId, capabilities);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceSetServer() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceSetServer(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    char            server[ISI_LONG_ADDRESS_STRING_SZ + 1];
    int             server_length;
    ISI_ServerType  type;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);

    /* Get server */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)server,
            &server_length)) {
        return;
    }
    /* Get service type */
    ISI_xdrGetInteger(xdr_ptr, (int*)&type);

    switch (type) {
        case ISI_SERVER_TYPE_PROXY:
        case ISI_SERVER_TYPE_REGISTRAR:
            /*
                      * Check does the proxy from provisioning data have sip: prefix,
                      * add it if without 
                      */
            if (NULL == OSAL_strncasescan(server, ISI_ADDRESS_STRING_SZ,
                    "sip:")) {
                OSAL_snprintf(_ISI_ServerObj_ptr->service.szProxy,
                ISI_ADDRESS_STRING_SZ, "sip:%s", server);
            }
            else {
                OSAL_strncpy(_ISI_ServerObj_ptr->service.szProxy,
                        server, sizeof(_ISI_ServerObj_ptr->service.szProxy));
            }
            break;
        case ISI_SERVER_TYPE_OUTBOUND_PROXY:
            OSAL_strncpy(_ISI_ServerObj_ptr->service.szOutboundProxy, server,
                    sizeof(_ISI_ServerObj_ptr->service.szOutboundProxy));
            break;
        case ISI_SERVER_TYPE_CHAT:
            OSAL_strncpy(_ISI_ServerObj_ptr->service.imConfUri, server,
                    sizeof(_ISI_ServerObj_ptr->service.imConfUri));
        default:
            /* Don't care others */
            break;
    }

    /* Don't need set to protocol */
    ret = ISI_RETURN_OK;

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceSetPort() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceSetPort(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    int             port;
    int             poolSize;
    ISI_ServerType  type;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get port*/
    ISI_xdrGetInteger(xdr_ptr, (int*)&port);
    /* Get poolSize */
    ISI_xdrGetInteger(xdr_ptr, (int*)&poolSize);
    /* Get service type */
    ISI_xdrGetInteger(xdr_ptr, (int*)&type);

    ret = ISI_serviceSetPort(serviceId, port, poolSize, type);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceSetIpsec() ========
 * This function is used to map the ISI functions.
 *
 * Return Values:
 *   None
 */
void _ISI_serverServiceSetIpsec(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    int             port;
    int             portPoolSize;
    int             spi;
    int             spiPoolSize;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get UC port */
    ISI_xdrGetInteger(xdr_ptr, (int*)&port);
    /* Get Uc port poolSize */
    ISI_xdrGetInteger(xdr_ptr, (int*)&portPoolSize);
    /* Get SPI */
    ISI_xdrGetInteger(xdr_ptr, (int*)&spi);
    /* Get SPI port poolSize */
    ISI_xdrGetInteger(xdr_ptr, (int*)&spiPoolSize);

    ret = ISI_serviceSetIpsec(serviceId,
            port, portPoolSize,
            spi, spiPoolSize);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceSetCredentials() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceSetCredentials(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    int             username_length;
    int             password_length;
    int             realm_length;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get username */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr,
            (char *)_ISI_ServerObj_ptr->service.szUsername,
            &username_length)) {
        return;
    }
    /* Get password */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr,
            (char *)_ISI_ServerObj_ptr->service.szPassword,
            &password_length)) {
        return;
    }
    /* Get realm */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr,
            (char *)_ISI_ServerObj_ptr->service.szRealm,
            &realm_length)) {
        return;
    }

    /* No need to call ISI api */
    ret = ISI_RETURN_OK;

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceSetDeviceId() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceSetDeviceId(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    char            instanceId[ISI_ADDRESS_STRING_SZ + 1];
    int             instanceId_length;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get uri */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)instanceId,
            &instanceId_length)) {
        return;
    }

    ret = ISI_serviceSetInstanceId(serviceId, instanceId);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceBlockUser() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceBlockUser(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    char            username[ISI_ADDRESS_STRING_SZ + 1];
    int             username_length;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get username */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)username,
            &username_length)) {
        return;
    }
    ret = ISI_serviceBlockUser(serviceId, username);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceUnblockUser() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceUnblockUser(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    char            username[ISI_ADDRESS_STRING_SZ];
    int             username_length;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get username */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)username,
            &username_length)) {
        return;
    }
    ret = ISI_serviceUnblockUser(serviceId, username);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAddCoderToService() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAddCoderToService(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    char            coderName[ISI_CODER_STRING_SZ + 1];
    int             coderName_length;
    char            coderDescription[ISI_CODER_DESCRIPTION_STRING_SZ + 1];
    int             description_length;

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get coderName */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)coderName,
            &coderName_length)) {
        return;
    }
    /* Get coderDescription */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)coderDescription,
            &description_length)) {
        return;
    }
    ret = ISI_addCoderToService(serviceId, coderName, coderDescription);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverRemoveCoderFromService() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverRemoveCoderFromService(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    char            coderName[ISI_CODER_STRING_SZ + 1];
    int             coderName_length;


    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get coderName */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)coderName,
            &coderName_length)) {
        return;
    }

    ret = ISI_removeCoderFromService(serviceId, coderName);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceForwardCalls() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceForwardCalls(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          evtId;
    ISI_Id          serviceId;
    ISI_FwdCond     condition;
    int             enable;
    char            to[ISI_CODER_STRING_SZ + 1];
    int             to_length;
    int             timeout;
    char            normalizedAddress[ISI_ADDRESS_STRING_SZ + 1];

    /* Get service id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get condition */
    ISI_xdrGetInteger(xdr_ptr, (int*)&condition);
    /* Get enable */
    ISI_xdrGetInteger(xdr_ptr, (int*)&enable);
    /* Get to */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)to,
            &to_length)) {
        return;
    }
    /* Normalize the remote address */
    RPM_normalizeOutboundAddress(_ISI_ServerObj_ptr->service.szDomain, to,
            normalizedAddress, ISI_ADDRESS_STRING_SZ,
            RPM_FEATURE_TYPE_CALL_NORMAL);
    /* Get timeout */
    ISI_xdrGetInteger(xdr_ptr, (int*)&timeout);

    ret = ISI_serviceForwardCalls(&evtId, serviceId, condition, enable,
            normalizedAddress, timeout);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put evtId */
    ISI_xdrPutInteger(xdr_ptr, evtId);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSendTelEventToRemote() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSendTelEventToRemote(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        evtId;
    ISI_Id        callId;
    ISI_TelEvent  telEvent;
    int           arg0;
    int           arg1;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get telEvent */
    ISI_xdrGetInteger(xdr_ptr, (int*)&telEvent);
    /* Get arg0 */
    ISI_xdrGetInteger(xdr_ptr, (int*)&arg0);
    /* Get arg1 */
    ISI_xdrGetInteger(xdr_ptr, (int*)&arg1);
    
    ret = ISI_sendTelEventToRemote(&evtId, callId, telEvent, arg0, arg1);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put return value of evtId */
    ISI_xdrPutInteger(xdr_ptr, evtId);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSendTelEventStringToRemote() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSendTelEventStringToRemote(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        evtId;
    ISI_Id        callId;
    ISI_TelEvent  telEvent;
    char          dtmfString[ISI_TEL_EVENT_STRING_SZ + 1];
    int           dtmf_length;
    int           arg1;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get telEvent */
    ISI_xdrGetInteger(xdr_ptr, (int*)&telEvent);
    /* Get teleEvent string */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)dtmfString,
            &dtmf_length)) {
        return;
    }
    /* Get arg1 */
    ISI_xdrGetInteger(xdr_ptr, (int*)&arg1);
    
    ret = ISI_sendTelEventStringToRemote(&evtId, callId, telEvent, 
        dtmfString, arg1);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put return value of evtId */
    ISI_xdrPutInteger(xdr_ptr, evtId);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetTelEventFromRemote() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetTelEventFromRemote(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        evtId;
    ISI_Id        callId;
    ISI_TelEvent  telEvent;
    int           arg0;
    int           arg1;
    char          from[ISI_ADDRESS_STRING_SZ + 1];
    char          dateTime[ISI_DATE_TIME_STRING_SZ + 1];

    /* Get evt id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&evtId);

    ret = ISI_getTelEventFromRemote(evtId, &callId, &telEvent, &arg0, &arg1,
            from, dateTime);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put return value of callId */
    ISI_xdrPutInteger(xdr_ptr, callId);
    /* put return value of telEvent */
    ISI_xdrPutInteger(xdr_ptr, telEvent);
    /* put return value of arg0 */
    ISI_xdrPutInteger(xdr_ptr, arg0);
    /* put return value of arg1 */
    ISI_xdrPutInteger(xdr_ptr, arg1);
    /* put from */
    ISI_xdrPutString(xdr_ptr, from, OSAL_strlen(from) + 1);
    /* put dateTime */
    ISI_xdrPutString(xdr_ptr, dateTime, OSAL_strlen(dateTime) + 1);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetTelEventResponse() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetTelEventResponse(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        evtId;
    ISI_TelEvent  telEvent;


    /* Get evt id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&evtId);

    ret = ISI_getTelEventResponse(evtId, &telEvent);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put return value of telEvent */
    ISI_xdrPutInteger(xdr_ptr, telEvent);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSendUSSD() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSendUSSD(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        evtId;
    ISI_Id        serviceId;
    char          ussd[ISI_ADDRESS_STRING_SZ + 1];
    int           ussd_length;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get to ussd */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)ussd, &ussd_length)) {
        return;
    }
    ret = ISI_sendUSSD(&evtId, serviceId, ussd);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put return value of evtId */
    ISI_xdrPutInteger(xdr_ptr, evtId);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverTerminateCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverTerminateCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    char          reason[ISI_EVENT_DESC_STRING_SZ + 1];
    int           reason_length;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get to reason */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reason,
            &reason_length)) {
        return;
    }

    ret = ISI_terminateCall(callId, reason);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverModifyCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverModifyCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    
    ret = ISI_modifyCall(callId, NULL);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAcceptCallModify() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAcceptCallModify(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    
    ret = ISI_acceptCallModify(callId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverRejectCallModify() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverRejectCallModify(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    char          reason[ISI_SUBJECT_STRING_SZ + 1];
    int           reason_length;
    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get to reason */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reason,
            &reason_length)) {
        return;
    }
    ret = ISI_rejectCallModify(callId, reason);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAddCoderToCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAddCoderToCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    char          coderName[ISI_CODER_STRING_SZ];
    char          coderDescription[ISI_CODER_DESCRIPTION_STRING_SZ + 1];
    int           coderName_len;
    int           coderDescription_len;
    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get coderName */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)coderName,
            &coderName_len)) {
        return;
    }
    /* Get coderDescription */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)coderDescription,
            &coderDescription_len)) {
        return;
    }
    ret = ISI_addCoderToCall(callId, coderName, coderDescription);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverRemoveCoderFromCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverRemoveCoderFromCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    char          coderName[ISI_CODER_STRING_SZ];
    int           coderName_len;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get coderName */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)coderName,
            &coderName_len)) {
        return;
    }

    ret = ISI_removeCoderFromCall(callId, coderName);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetCallState() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetCallState(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    ISI_CallState callState;
    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    
    ret = ISI_getCallState(callId, &callState);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put return value of callState */
    ISI_xdrPutInteger(xdr_ptr, callState);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSetCallSessionDirection() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSetCallSessionDirection(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return           ret;
    ISI_Id               callId;
    ISI_SessionType      callType;
    ISI_SessionDirection direction;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get callType */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callType);
    /* Get direction */
    ISI_xdrGetInteger(xdr_ptr, (int*)&direction);
    
    ret = ISI_setCallSessionDirection(callId, callType, direction);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetCallSessionDirection() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetCallSessionDirection(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return           ret;
    ISI_Id               callId;
    ISI_SessionType      callType;
    ISI_SessionDirection callDir;
    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get callType */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callType);

    ret = ISI_getCallSessionDirection(callId, callType, &callDir);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put return value of callDir */
    ISI_xdrPutInteger(xdr_ptr, callDir);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetCallHeader() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetCallHeader(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    char          subject[ISI_SUBJECT_STRING_SZ + 1];
    char          from[ISI_ADDRESS_STRING_SZ + 1];
    
    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    
    ret = ISI_getCallHeader(callId, subject, from);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put subject */
    ISI_xdrPutString(xdr_ptr, subject, OSAL_strlen(subject) + 1);
    /* put from */
    ISI_xdrPutString(xdr_ptr, from, OSAL_strlen(from) + 1);
   
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}


/*
 * ======== _ISI_serverGetSupsrvHeader() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetSupsrvHeader(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    int           supsrvHfExist;
    
    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    
    ret = ISI_getSupsrvHeader(callId, &supsrvHfExist);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put supsrvHfExist */
    ISI_xdrPutInteger(xdr_ptr, supsrvHfExist);
   
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetSupsrvHistoryInfo() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetSupsrvHistoryInfo(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    char          historyInfo[ISI_HISTORY_INFO_STRING_SZ+1];
    
    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    
    ret = ISI_getSupsrvHistoryInfo(callId, historyInfo);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put historyInfo */
    ISI_xdrPutString(xdr_ptr, historyInfo, OSAL_strlen(historyInfo) + 1);
   
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetCallSessionType() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetCallSessionType(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return        ret;
    ISI_Id            callId;
    uint16            callType;
    
    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    
    ret = ISI_getCallSessionType(callId, &callType);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put callType */
    ISI_xdrPutInteger(xdr_ptr, callType);
   
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetCoderDescription() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetCoderDescription(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    char          coderName[ISI_CODER_STRING_SZ];
    char          coderDescription[ISI_CODER_DESCRIPTION_STRING_SZ + 1];
    int           coderName_len;
    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get coderName */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)coderName,
            &coderName_len)) {
        return;
    }
    ret = ISI_getCoderDescription(callId, coderName, coderDescription);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put coderDescription */
    ISI_xdrPutString(xdr_ptr, coderDescription,
        OSAL_strlen(coderDescription) + 1);
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAcknowledgeCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAcknowledgeCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    
    ret = ISI_acknowledgeCall(callId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverHoldCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverHoldCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    
    ret = ISI_holdCall(callId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverResumeCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverResumeCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    
    ret = ISI_resumeCall(callId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAcceptCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAcceptCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    
    ret = ISI_acceptCall(callId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverRejectCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverRejectCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    char          reason[ISI_EVENT_DESC_STRING_SZ + 1];
    int           reason_length;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get to reason */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reason,
            &reason_length)) {
        return;
    }
    
    ret = ISI_rejectCall(callId, reason);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverForwardCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverForwardCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    char          to[ISI_ADDRESS_STRING_SZ + 1];
    int           to_length;
    char          normalizedAddress[ISI_ADDRESS_STRING_SZ + 1];

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get to uri */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)to, &to_length)) {
        return;
    }
    /* Normalize the remote address */
    RPM_normalizeOutboundAddress(_ISI_ServerObj_ptr->service.szDomain, to,
            normalizedAddress, ISI_ADDRESS_STRING_SZ,
            RPM_FEATURE_TYPE_CALL_NORMAL);
    ret = ISI_forwardCall(callId, normalizedAddress);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverBlindTransferCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverBlindTransferCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    char          to[ISI_ADDRESS_STRING_SZ + 1];
    int           to_length;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get to uri */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)to, &to_length)) {
        return;
    }
    ret = ISI_blindTransferCall(callId, to);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAttendedTransferCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAttendedTransferCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    char          to[ISI_ADDRESS_STRING_SZ + 1];
    int           to_length;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get to uri */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)to, &to_length)) {
        return;
    }
    ret = ISI_attendedTransferCall(callId, to);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverConsultativeTransferCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverConsultativeTransferCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    ISI_Id        toCallId;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get  to callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&toCallId);
    
    ret = ISI_consultativeTransferCall(callId, toCallId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGenerateTone() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGenerateTone(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;
    ISI_AudioTone tone;
    int           duration;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    /* Get tone */
    ISI_xdrGetInteger(xdr_ptr, (int*)&tone);
    /* Get  duration */
    ISI_xdrGetInteger(xdr_ptr, (int*)&duration);
    
    ret = ISI_generateTone(callId, tone, duration);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverStopTone() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverStopTone(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        callId;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);

    ret = ISI_stopTone(callId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverStartConfCall() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverStartConfCall(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id  confId;
    ISI_Id  callId0;
    ISI_Id  callId1;

    /* Get tone */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId0);
    /* Get  duration */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId1);
    
    ret = ISI_startConfCall(&confId, callId0, callId1);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put  confId */
    ISI_xdrPutInteger(xdr_ptr, confId);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAddCallToConf() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAddCallToConf(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id  confId;
    ISI_Id  callId;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&confId);
    /* Get tone */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    
    ret = ISI_addCallToConf(confId, callId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverRemoveCallFromConf() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverRemoveCallFromConf(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id  confId;
    ISI_Id  callId;

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&confId);
    /* Get tone */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);
    
    ret = ISI_removeCallFromConf(confId, callId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSendMessage() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSendMessage(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             msgId;
    ISI_Id             serviceId;
    ISI_MessageType    type;
    char               to[ISI_ADDRESS_STRING_SZ + 1];
    int                to_length;
    char               subject[ISI_SUBJECT_STRING_SZ + 1];
    int                subject_length;
    char               msg[ISI_TEXT_STRING_SZ + 1];
    int                msgLen;
    ISI_MessageReport  report;
    char               reportId[ISI_ID_STRING_SZ + 1];
    int                reportId_length;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get type */
    ISI_xdrGetInteger(xdr_ptr, (int*)&type);
    /* Get to */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)to, &to_length)) {
        return;
    }
    /* Get subject */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)subject,
            &subject_length)) {
        return;
    }
    /* Get msg */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)msg, &msgLen)) {
        return;
    }
    /* Get report */
    ISI_xdrGetInteger(xdr_ptr, (int*)&report);
    /* Get reportId */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reportId,
            &reportId_length)) {
        return;
    }

    ret = ISI_sendMessage(&msgId, serviceId, type, to, subject, msg, msgLen,
            report, reportId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put msgId */
    ISI_xdrPutInteger(xdr_ptr, msgId);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }

    /* to put the message Id into the database for tracing it */
    if (ISI_RETURN_OK == ret) {
        _ISI_serverDbEntryAdd(ISI_ID_TYPE_MESSAGE, msgId);
    }
}

/*
 * ======== _ISI_serverSendMessageReport() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSendMessageReport(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             msgId;
    ISI_Id             serviceId;
    char               to[ISI_ADDRESS_STRING_SZ + 1];
    int                to_length;
    ISI_MessageReport  report;
    char               reportId[ISI_ID_STRING_SZ + 1];
    int                reportId_length;
    char               normalizedAddress[ISI_ADDRESS_STRING_SZ + 1];

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get to */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)to, &to_length)) {
        return;
    }
    /* Normalize the remote address */
    RPM_normalizeOutboundAddress(_ISI_ServerObj_ptr->service.szDomain, to,
            normalizedAddress, ISI_ADDRESS_STRING_SZ,
            RPM_FEATURE_TYPE_RCS);
    /* Get report */
    ISI_xdrGetInteger(xdr_ptr, (int*)&report);
    /* Get reportId */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reportId,
            &reportId_length)) {
        return;
    }

    ret = ISI_sendMessageReport(&msgId, serviceId, normalizedAddress, report,
            reportId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put msgId */
    ISI_xdrPutInteger(xdr_ptr, msgId);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }

    /* to put the message Id into the database for tracing it */
    if (ISI_RETURN_OK == ret) {
        _ISI_serverDbEntryAdd(ISI_ID_TYPE_MESSAGE, msgId);
    }
}

/*
 * ======== _ISI_serverReadMessageReport() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverReadMessageReport(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             msgId;
    ISI_Id             chatId;
    char               from[ISI_ADDRESS_STRING_SZ + 1];
    char               dateTime[ISI_DATE_TIME_STRING_SZ + 1];
    ISI_MessageReport  report;
    char               reportId[ISI_ID_STRING_SZ + 1];

    /* Get msgId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&msgId);

    ret = ISI_readMessageReport(msgId, &chatId, from, dateTime, &report,
            reportId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put chatId */
    ISI_xdrPutInteger(xdr_ptr, chatId);
    /* put from */
    ISI_xdrPutString(xdr_ptr, from, OSAL_strlen(from) + 1);
    /* put dateTime */
    ISI_xdrPutString(xdr_ptr, dateTime, OSAL_strlen(dateTime) + 1);
    /* put report */
    ISI_xdrPutInteger(xdr_ptr, report);
    /* put reportId */
    ISI_xdrPutString(xdr_ptr, reportId, OSAL_strlen(reportId) + 1);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetMessageHeader() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetMessageHeader(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             msgId;
    ISI_MessageType    type;
    char               subject[ISI_SUBJECT_STRING_SZ + 1];
    char               from[ISI_ADDRESS_STRING_SZ + 1];
    char               dateTime[ISI_DATE_TIME_STRING_SZ + 1];
    ISI_MessageReport  report;
    char               reportId[ISI_ID_STRING_SZ + 1];

    /* Get msgId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&msgId);

    ret = ISI_getMessageHeader(msgId, &type, subject, from, dateTime, &report,
            reportId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put type */
    ISI_xdrPutInteger(xdr_ptr, type);
    /* put subject */
    ISI_xdrPutString(xdr_ptr, subject, OSAL_strlen(subject) + 1);
    /* put from */
    ISI_xdrPutString(xdr_ptr, from, OSAL_strlen(from) + 1);
    /* put dateTime */
    ISI_xdrPutString(xdr_ptr, dateTime, OSAL_strlen(dateTime) + 1);
    /* put report */
    ISI_xdrPutInteger(xdr_ptr, report);
    /* put reportId */
    ISI_xdrPutString(xdr_ptr, reportId, OSAL_strlen(reportId) + 1);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverReadMessage() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverReadMessage(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             msgId;
    ISI_Id             chatId;
    char               part[ISI_TEXT_STRING_SZ + 1];
    int                len;

    /* Get msgId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&msgId);
    /* Get len */
    ISI_xdrGetInteger(xdr_ptr, (int*)&len);

    ret = ISI_readMessage(msgId, &chatId, part, &len);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put chatId */
    ISI_xdrPutInteger(xdr_ptr, chatId);
    /* put part */
    ISI_xdrPutString(xdr_ptr, part, OSAL_strlen(part) + 1);
    /* put len */
    ISI_xdrPutInteger(xdr_ptr, len);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSendFile() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSendFile(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             fileId;
    ISI_Id             serviceId;
    char               to[ISI_ADDRESS_STRING_SZ + 1];
    int                to_length;
    char               subject[ISI_SUBJECT_STRING_SZ + 1];
    int                subject_length;
    char               filePath[ISI_FILE_PATH_STRING_SZ + 1];
    int                filePath_length;
    ISI_FileType       fileType;
    ISI_FileAttribute  attribute;
    char               normalizedAddress[ISI_ADDRESS_STRING_SZ + 1];
    int                fileSize;
    int                report;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get to */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)to, &to_length)) {
        return;
    }
    /* Normalize the remote address */
    RPM_normalizeOutboundAddress(_ISI_ServerObj_ptr->service.szDomain, to,
            normalizedAddress, ISI_ADDRESS_STRING_SZ,
            RPM_FEATURE_TYPE_RCS);
    /* Get subject */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)subject,
            &subject_length)) {
        return;
    }
    /* Get filePath */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)filePath,
            &filePath_length)) {
        return;
    }

    /* Get fileType */
    ISI_xdrGetInteger(xdr_ptr, (int*)&fileType);
    /* Get attribute */
    ISI_xdrGetInteger(xdr_ptr, (int*)&attribute);
    /* Get fileSize */
    ISI_xdrGetInteger(xdr_ptr, (int*)&fileSize);
    /* Get report */
    ISI_xdrGetInteger(xdr_ptr, (int*)&report);

    ret = ISI_sendFile(&fileId, serviceId, normalizedAddress, subject,
            filePath, fileType, attribute, fileSize, report);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put fileId */
    ISI_xdrPutInteger(xdr_ptr, fileId);
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAcceptFile() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAcceptFile(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             fileId; 

    /* Get fileId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&fileId);

    ret = ISI_acceptFile(fileId);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret); 
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAcknowledgeFile() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAcknowledgeFile(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             fileId; 

    /* Get fileId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&fileId);

    ret = ISI_acknowledgeFile(fileId);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret); 
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverRejectFile() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverRejectFile(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        fileId; 
    char          reason[ISI_EVENT_DESC_STRING_SZ + 1];
    int           reason_length;
    /* Get fileId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&fileId);
    /* Get reason */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reason,
            &reason_length)) {
        return;
    }
    ret = ISI_rejectFile(fileId, reason);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret); 
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverCancelFile() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverCancelFile(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return    ret;
    ISI_Id        fileId; 
    char          reason[ISI_EVENT_DESC_STRING_SZ + 1];
    int           reason_length;
    /* Get fileId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&fileId);
    /* Get reason */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reason,
            &reason_length)) {
        return;
    }
    ret = ISI_cancelFile(fileId, reason);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret); 
 
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetFileHeader() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetFileHeader(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             fileId;
    char               subject[ISI_SUBJECT_STRING_SZ + 1];
    char               from[ISI_ADDRESS_STRING_SZ + 1];

    /* Get msgId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&fileId);

    ret = ISI_getFileHeader(fileId, subject, from);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put subject */
    ISI_xdrPutString(xdr_ptr, subject, OSAL_strlen(subject) + 1);
    /* put from */
    ISI_xdrPutString(xdr_ptr, from, OSAL_strlen(from) + 1);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverReadFileProgress() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverReadFileProgress(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             fileId;
    char               filePath[ISI_FILE_PATH_STRING_SZ + 1];
    ISI_FileType       fileType;
    ISI_FileAttribute  fileAttribute;
    vint               fileSize;
    vint               progress;

    /* Get fileId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&fileId);
    ret = ISI_readFileProgress(fileId, filePath, &fileType, &fileAttribute,
            &fileSize, &progress);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put fileId */
    ISI_xdrPutInteger(xdr_ptr, fileId);
    /* put filePath */
    ISI_xdrPutString(xdr_ptr, filePath, OSAL_strlen(filePath) + 1);
    /* put fileType */
    ISI_xdrPutInteger(xdr_ptr, fileType);
    /* put fileAttribute */
    ISI_xdrPutInteger(xdr_ptr, fileAttribute);
    /* put fileSize */
    ISI_xdrPutInteger(xdr_ptr, fileSize);
    /* put progress */
    ISI_xdrPutInteger(xdr_ptr, progress);
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAddContact() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAddContact(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           presId;
    ISI_Id           serviceId;
    char             user[ISI_ADDRESS_STRING_SZ + 1];
    int              user_len;
    char             group[ISI_ADDRESS_STRING_SZ + 1];
    int              group_len;
    char             name[ISI_ADDRESS_STRING_SZ + 1];
    int              name_len;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get user */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)user, &user_len)) {
        return;
    }
    /* Get group */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)group, &group_len)) {
        return;
    }
    /* Get name */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)name, &name_len)) {
        return;
    }

    ret = ISI_addContact(&presId, serviceId, user, group, name);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put presId */
    ISI_xdrPutInteger(xdr_ptr, presId);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverRemoveContact() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverRemoveContact(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           presId;
    ISI_Id           serviceId;
    char             user[ISI_ADDRESS_STRING_SZ + 1];
    int              user_len;
    char             group[ISI_ADDRESS_STRING_SZ + 1];
    int              group_len;
    char             name[ISI_ADDRESS_STRING_SZ + 1];
    int              name_len;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get user */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)user, &user_len)) {
        return;
    }
    /* Get group */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)group, &group_len)) {
        return;
    }
    /* Get name */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)name, &name_len)) {
        return;
    }

    ret = ISI_removeContact(&presId, serviceId, user, group, name);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put presId */
    ISI_xdrPutInteger(xdr_ptr, presId);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverRemoveContact() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverReadContact(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           presId;
    char             user[ISI_ADDRESS_STRING_SZ + 1];
    char             group[ISI_ADDRESS_STRING_SZ + 1];
    char             name[ISI_ADDRESS_STRING_SZ + 1];
    char             subscription[ISI_PRESENCE_STRING_SZ + 1];
    int              len;

    /* Get presId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&presId);
    /* Get len */
    ISI_xdrGetInteger(xdr_ptr, (int*)&len);
    
    ret = ISI_readContact(presId, user, group, name, subscription, len);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put user */
    ISI_xdrPutString(xdr_ptr, user, OSAL_strlen(user) + 1);
    /* put group */
    ISI_xdrPutString(xdr_ptr, group, OSAL_strlen(group) + 1);
    /* put name */
    ISI_xdrPutString(xdr_ptr, name, OSAL_strlen(name) + 1);
    /* put subscription */
    ISI_xdrPutString(xdr_ptr, subscription, OSAL_strlen(subscription) + 1);
    
    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSubscribeToPresence() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSubscribeToPresence(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           presId;
    ISI_Id           serviceId;
    char             user[ISI_ADDRESS_STRING_SZ + 1];
    int              user_len;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get user */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)user, &user_len)) {
        return;
    }

    ret = ISI_subscribeToPresence(&presId, serviceId, user);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put presId */
    ISI_xdrPutInteger(xdr_ptr, presId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverUnsubscribeFromPresence() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverUnsubscribeFromPresence(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           presId;
    ISI_Id           serviceId;
    char             user[ISI_ADDRESS_STRING_SZ + 1];
    int              user_len;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get user */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)user, &user_len)) {
        return;
    }

    ret = ISI_unsubscribeFromPresence(&presId, serviceId, user);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put presId */
    ISI_xdrPutInteger(xdr_ptr, presId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverReadSubscribeToPresenceRequest() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverReadSubscribeToPresenceRequest(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           presId;
    char             user[ISI_ADDRESS_STRING_SZ + 1];

    /* Get presId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&presId);

    ret = ISI_readSubscribeToPresenceRequest(presId, user);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put user */
    ISI_xdrPutString(xdr_ptr, user, OSAL_strlen(user) + 1);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverReadSubscriptionToPresenceResponse() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverReadSubscriptionToPresenceResponse(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           presId;
    char             user[ISI_ADDRESS_STRING_SZ + 1];
    int              isAllowed;

    /* Get presId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&presId);

    ret = ISI_readSubscriptionToPresenceResponse(presId, user, &isAllowed);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put user */
    ISI_xdrPutString(xdr_ptr, user, OSAL_strlen(user) + 1);
    /* put isAllowed */
    ISI_xdrPutInteger(xdr_ptr, isAllowed);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAllowSubscriptionToPresence() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAllowSubscriptionToPresence(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           presId;
    ISI_Id           serviceId;
    char             user[ISI_ADDRESS_STRING_SZ + 1];
    int              user_len;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get user */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)user, &user_len)) {
        return;
    }

    ret = ISI_allowSubscriptionToPresence(&presId, serviceId, user);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put presId */
    ISI_xdrPutInteger(xdr_ptr, presId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverDenySubscriptionToPresence() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverDenySubscriptionToPresence(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           presId;
    ISI_Id           serviceId;
    char             user[ISI_ADDRESS_STRING_SZ + 1];
    int              user_len;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get user */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)user, &user_len)) {
        return;
    }

    ret = ISI_denySubscriptionToPresence(&presId, serviceId, user);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put presId */
    ISI_xdrPutInteger(xdr_ptr, presId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSendPresence() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSendPresence(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           presId;
    ISI_Id           serviceId;
    char             to[ISI_ADDRESS_STRING_SZ + 1];
    int              to_len;
    char             presence[ISI_PRESENCE_STRING_SZ + 1];
    int              presence_len;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get to */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)to, &to_len)) {
        return;
    }
    /* Get presence */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)presence,
            &presence_len)) {
        return;
    }

    ret = ISI_sendPresence(&presId, serviceId, to, presence);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put presId */
    ISI_xdrPutInteger(xdr_ptr, presId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverReadPresence() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverReadPresence(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           presId;
    ISI_Id           callId;
    char             from[ISI_ADDRESS_STRING_SZ + 1];
    char             presence[ISI_PRESENCE_STRING_SZ + 1];
    int              len;

    /* Get presId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&presId);
    /* Get len */
    ISI_xdrGetInteger(xdr_ptr, (int*)&len);

    ret = ISI_readPresence(presId, &callId, from, presence, len);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put callId */
    ISI_xdrPutInteger(xdr_ptr, callId);
    /* put from */
    ISI_xdrPutString(xdr_ptr, from, OSAL_strlen(from) + 1);
    /* put presence */
    ISI_xdrPutString(xdr_ptr, presence, OSAL_strlen(presence) + 1);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSendCapabilities() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSendCapabilities(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           capsId;
    ISI_Id           serviceId;
    char             to[ISI_ADDRESS_STRING_SZ + 1];
    int              to_len;
    char             capabilities[ISI_PRESENCE_STRING_SZ + 1];
    int              capabilities_len;
    int              priority;
    char             normalizedTo[ISI_ADDRESS_STRING_SZ + 1];
 
    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get to */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)to, &to_len)) {
        return;
    }
    /* Normalize the to address */
    RPM_normalizeOutboundAddress(_ISI_ServerObj_ptr->service.szDomain, to,
            normalizedTo, ISI_ADDRESS_STRING_SZ,
            RPM_FEATURE_TYPE_RCS);
    /* Get capabilities */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)capabilities,
            &capabilities_len)) {
        return;
    }
    /* Get priority */
    ISI_xdrGetInteger(xdr_ptr, (int*)&priority);

    ret = ISI_sendCapabilities(&capsId, serviceId, normalizedTo, capabilities, priority);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put capsId */
    ISI_xdrPutInteger(xdr_ptr, capsId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverReadCapabilities() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverReadCapabilities(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           capsIdIn;
    ISI_Id           capsIdOut;
    char             from[ISI_ADDRESS_STRING_SZ + 1];
    char             capabilities[ISI_PRESENCE_STRING_SZ + 1];
    int              len;

    /* Get capsIdIn */
    ISI_xdrGetInteger(xdr_ptr, (int*)&capsIdIn);
    /* Get len */
    ISI_xdrGetInteger(xdr_ptr, (int*)&len);

    ret = ISI_readCapabilities(capsIdIn, &capsIdOut, from, capabilities, len);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put ISI return capsIdOut */
    ISI_xdrPutInteger(xdr_ptr, capsIdOut);
    /* put from */
    ISI_xdrPutString(xdr_ptr, from, OSAL_strlen(from) + 1);
    /* put capabilities */
    ISI_xdrPutString(xdr_ptr, capabilities, OSAL_strlen(capabilities) + 1);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetCallHandoff() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetCallHandoff(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           callId;
    char             to[ISI_ADDRESS_STRING_SZ + 1];

    /* Get callId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&callId);

    ret = ISI_getCallHandoff(callId, to);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put to*/
    ISI_xdrPutString(xdr_ptr, to, OSAL_strlen(to) + 1);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSetChatNickname() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSetChatNickname(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           serviceId;
    char             name[ISI_ADDRESS_STRING_SZ + 1];
    int              name_len;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get name */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)name,
            &name_len)) {
        return;
    }
    ret = ISI_setChatNickname(serviceId, name);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSetChatNickname() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverInitiateGroupChat(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           chatId;
    ISI_Id           serviceId;
    char             roomName[ISI_ADDRESS_STRING_SZ + 1];
    int              roomName_len;
    char             subject[ISI_SUBJECT_STRING_SZ + 1];
    int              subject_len;
    char             password[ISI_PASSWORD_SZ + 1];
    int              password_len;
    ISI_SessionType  type;
    char             normalizedAddress[ISI_ADDRESS_STRING_SZ + 1];

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get roomName */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)roomName,
            &roomName_len)) {
        return;
    }
    /* Normalize the remote address */
    RPM_normalizeOutboundAddress(_ISI_ServerObj_ptr->service.szDomain, roomName,
            normalizedAddress, ISI_ADDRESS_STRING_SZ,
            RPM_FEATURE_TYPE_RCS);
    /* Get subject */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)subject,
            &subject_len)) {
        return;
    }
    /* Get password */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)password,
            &password_len)) {
        return;
    }
    /* Get session type */
    ISI_xdrGetInteger(xdr_ptr, (int*)&type);

    /* Get current network mode and set the session type */
    if (ISI_NETWORK_MODE_LTE == _ISI_ServerObj_ptr->networkMode) {
        type = _ISI_ServerObj_ptr->psMediaSessionType;
    }
    else if (ISI_NETWORK_MODE_WIFI == _ISI_ServerObj_ptr->networkMode) {
        type = _ISI_ServerObj_ptr->wifiMediaSessionType;
    }
    else {
        return;
    }

    ret = ISI_initiateGroupChat(&chatId, serviceId, normalizedAddress, subject,
            password, type);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put chatId */
    ISI_xdrPutInteger(xdr_ptr, chatId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_normalizeParticipants() ========
 *
 * Private function to normalize each participants.
 * 
 * Returns: 
 * OSAL_TRUE      : Normalize success.
 * OSAL_FALSE     : Normalize failed.
 */
void _ISI_normalizeParticipants(
    char *in_ptr,
    char *out_ptr)
{
    char *ret_ptr;
    char normalizedAddress[ISI_ADDRESS_STRING_SZ + 1];
    char *pos_ptr;

    /* find the first item by ',' */
    ret_ptr = OSAL_strtok(in_ptr, ",");
    pos_ptr = out_ptr;
    while (NULL != ret_ptr) {
        /* Normalize the remote address */
        RPM_normalizeOutboundAddress(_ISI_ServerObj_ptr->service.szDomain, ret_ptr,
                normalizedAddress, ISI_ADDRESS_STRING_SZ, RPM_FEATURE_TYPE_RCS);
        /* add normalized result and ',' */
        OSAL_snprintf(pos_ptr, OSAL_strlen(normalizedAddress) + 2, "%s,",
                    normalizedAddress);
        pos_ptr  = out_ptr + OSAL_strlen(out_ptr);
        /* Find next ',' */
        ret_ptr = OSAL_strtok(NULL, ",");
    }
    /* replace ',' by 0 to the end of output  */
    *(pos_ptr - 1) = '\0';
    ISI_rpcDbgPrintf("Normalized Participants=%s", out_ptr);
 
}

/*
 * ======== _ISI_serverInitiateGroupChatAdhoc() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverInitiateGroupChatAdhoc(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return       ret;
    ISI_Id           chatId;
    ISI_Id           serviceId;
    char             participant[ISI_LONG_ADDRESS_STRING_SZ + 1];
    int              participant_len;
    char             conferenceUri[ISI_ADDRESS_STRING_SZ + 1];
    int              conferenceUri_len;
    char             subject[ISI_SUBJECT_STRING_SZ + 1];
    int              subject_len;
    ISI_SessionType  type;
    char             normalizedParticipants[ISI_LONG_ADDRESS_STRING_SZ + 1];

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get roomName */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)participant,
            &participant_len)) {
        return;
    }

    /* Get the conference Uri */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)conferenceUri,
            &conferenceUri_len)) {
        return;
    }

    /* Get subject */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)subject,
            &subject_len)) {
        return;
    }

    /* Get session type */
    ISI_xdrGetInteger(xdr_ptr, (int*)&type);

    /* Get current network mode and set the session type */
    if (ISI_NETWORK_MODE_LTE == _ISI_ServerObj_ptr->networkMode) {
        type = _ISI_ServerObj_ptr->psMediaSessionType;
    }
    else if (ISI_NETWORK_MODE_WIFI == _ISI_ServerObj_ptr->networkMode) {
        type = _ISI_ServerObj_ptr->wifiMediaSessionType;
    }
    else {
        return;
    }
    OSAL_memSet(normalizedParticipants, 0, ISI_LONG_ADDRESS_STRING_SZ);
    /* To normalize participants */
    _ISI_normalizeParticipants(participant, normalizedParticipants);
    ret = ISI_initiateGroupChatAdhoc(&chatId, serviceId, normalizedParticipants,
            conferenceUri, subject, type);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put chatId */
    ISI_xdrPutInteger(xdr_ptr, chatId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverInitiateChat() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverInitiateChat(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             chatId;
    ISI_Id             serviceId;
    char               participant[ISI_ADDRESS_STRING_SZ + 1];
    int                participant_len;
    char               subject[ISI_SUBJECT_STRING_SZ + 1];
    int                subject_len;
    char               message[ISI_TEXT_STRING_SZ + 1];
    int                message_len;
    ISI_MessageReport  reports;
    char               reportId[ISI_ID_STRING_SZ + 1];
    int                reportId_len;
    ISI_SessionType    type;
    char               normalizedAddress[ISI_ADDRESS_STRING_SZ + 1];

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get roomName */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)participant,
            &participant_len)) {
        return;
    }
    /* Normalize the remote address */
    RPM_normalizeOutboundAddress(_ISI_ServerObj_ptr->service.szDomain, participant,
            normalizedAddress, ISI_ADDRESS_STRING_SZ,
            RPM_FEATURE_TYPE_RCS);
    /* Get subject */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)subject,
            &subject_len)) {
        return;
    }
    /* Get message */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)message,
            &message_len)) {
        return;
    }
    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&reports);
    /* Get subject */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reportId,
            &reportId_len)) {
        return;
    }

    /* Get session type */
    ISI_xdrGetInteger(xdr_ptr, (int*)&type);

    /* Get current network mode and set the session type */
    if (ISI_NETWORK_MODE_LTE == _ISI_ServerObj_ptr->networkMode) {
        type = _ISI_ServerObj_ptr->psMediaSessionType;
    }
    else if (ISI_NETWORK_MODE_WIFI == _ISI_ServerObj_ptr->networkMode) {
        type = _ISI_ServerObj_ptr->wifiMediaSessionType;
    }
    else {
        return;
    }

    ret = ISI_initiateChat(&chatId, serviceId, normalizedAddress, subject, message,
            reports, reportId, type);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put chatId */
    ISI_xdrPutInteger(xdr_ptr, chatId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAcceptChat() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAcceptChat(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             chatId;

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);

    ret = ISI_acceptChat(chatId);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverRejectChat() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverRejectChat(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             chatId;
    char          reason[ISI_EVENT_DESC_STRING_SZ + 1];
    int           reason_length;

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);
    /* Get reason */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reason,
            &reason_length)) {
        return;
    }

    ret = ISI_rejectChat(chatId, reason);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverAcknowledgeChat() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverAcknowledgeChat(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             chatId;

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);

    ret = ISI_acknowledgeChat(chatId);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverDisconnectChat() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverDisconnectChat(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             chatId;

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);

    ret = ISI_disconnectChat(chatId);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetChatHeader() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetChatHeader(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             chatId;
    char               subject[ISI_SUBJECT_STRING_SZ + 1];
    char               remoteAddress[ISI_ADDRESS_STRING_SZ + 1];
    ISI_Id             messageId;

    /* Get msgId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);

    ret = ISI_getChatHeader(chatId, subject, remoteAddress, &messageId);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put subject */
    ISI_xdrPutString(xdr_ptr, subject, OSAL_strlen(subject) + 1);
    /* put from */
    ISI_xdrPutString(xdr_ptr, remoteAddress, OSAL_strlen(remoteAddress) + 1);
    /* put messageId */
    ISI_xdrPutInteger(xdr_ptr, messageId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetGroupChatHeader() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetGroupChatHeader(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             chatId;
    char               subject[ISI_SUBJECT_STRING_SZ + 1];
    char               conferenceUri[ISI_ADDRESS_STRING_SZ + 1];
    char               participants[ISI_ADDRESS_STRING_SZ + 1];

    /* Get msgId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);

    ret = ISI_getGroupChatHeader(chatId, subject, conferenceUri, participants);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put subject */
    ISI_xdrPutString(xdr_ptr, subject, OSAL_strlen(subject) + 1);
    /* put conferenceUri */
    ISI_xdrPutString(xdr_ptr, conferenceUri, OSAL_strlen(conferenceUri) + 1);
    /* put participants */
    ISI_xdrPutString(xdr_ptr, participants, OSAL_strlen(participants) + 1);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverReadGroupChatPresence() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverReadGroupChatPresence(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return   ret;
    ISI_Id       presId;
    ISI_Id       chatId;
    char         from[ISI_SUBJECT_STRING_SZ + 1];
    char         presence[ISI_PRESENCE_STRING_SZ + 1];
    int          len;

    /* Get presId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&presId);
    /* Get len */
    ISI_xdrGetInteger(xdr_ptr, (int*)&len);
    ret = ISI_readGroupChatPresence(presId, &chatId, from, presence, len);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put chatId */
    ISI_xdrPutInteger(xdr_ptr, chatId);
    /* put from */
    ISI_xdrPutString(xdr_ptr, from, OSAL_strlen(from) + 1);
    /* put presence */
    ISI_xdrPutString(xdr_ptr, presence, OSAL_strlen(presence) + 1);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverInviteGroupChat() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverInviteGroupChat(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return   ret;
    ISI_Id       chatId;
    char         participant[ISI_ADDRESS_STRING_SZ + 1];
    int          participant_len;
    char         reason[ISI_EVENT_DESC_STRING_SZ + 1];
    int          reason_len;
    char         normalizedParticipants[ISI_LONG_ADDRESS_STRING_SZ + 1];

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);
    /* Get participant */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)participant,
            &participant_len)) {
        return;
    }
    OSAL_memSet(normalizedParticipants, 0, ISI_LONG_ADDRESS_STRING_SZ);
    /* To normalize participants */
    _ISI_normalizeParticipants(participant, normalizedParticipants);
    /* Get reason */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reason,
            &reason_len)) {
        return;
    }
    ret = ISI_inviteGroupChat(chatId, normalizedParticipants, reason);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverJoinGroupChat() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverJoinGroupChat(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          chatId;
    ISI_Id          serviceId;
    char            remoteAddress[ISI_ADDRESS_STRING_SZ + 1];
    int             remoteAddress_len;
    char            password[ISI_PASSWORD_SZ + 1];
    int             password_len;
    ISI_SessionType type;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get remoteAddress */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)remoteAddress,
            &remoteAddress_len)) {
        return;
    }
    /* Get password */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)password,
            &password_len)) {
        return;
    }
    /* Get session type */
    ISI_xdrGetInteger(xdr_ptr, (int*)&type);

    /* Get current network mode and set the session type */
    if (ISI_NETWORK_MODE_LTE == _ISI_ServerObj_ptr->networkMode) {
        type = _ISI_ServerObj_ptr->psMediaSessionType;
    }
    else if (ISI_NETWORK_MODE_WIFI == _ISI_ServerObj_ptr->networkMode) {
        type = _ISI_ServerObj_ptr->wifiMediaSessionType;
    }
    else {
        return;
    }

    ret = ISI_joinGroupChat(&chatId, serviceId, remoteAddress, password, type);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put chatId */
    ISI_xdrPutInteger(xdr_ptr, chatId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverKickGroupChat() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverKickGroupChat(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return   ret;
    ISI_Id       chatId;
    char         participant[ISI_ADDRESS_STRING_SZ + 1];
    int          participant_len;
    char         reason[ISI_EVENT_DESC_STRING_SZ + 1];
    int          reason_len;

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);
    /* Get participant */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)participant,
            &participant_len)) {
        return;
    }
    /* Get reason */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reason,
            &reason_len)) {
        return;
    }
    ret = ISI_kickGroupChat(chatId, participant, reason);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverDestroyGroupChat() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverDestroyGroupChat(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return   ret;
    ISI_Id       chatId;
    char         reason[ISI_EVENT_DESC_STRING_SZ + 1];
    int          reason_len;

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);
    /* Get reason */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reason,
            &reason_len)) {
        return;
    }
    ret = ISI_destroyGroupChat(chatId, reason);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSendChatMessage() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSendChatMessage(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             msgId;
    ISI_Id             chatId;
    char               msg[ISI_TEXT_STRING_SZ + 1];
    int                msg_len;
    ISI_MessageReport  report;
    char               reportId[ISI_ID_STRING_SZ + 1];
    int                reportId_len;

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);
    /* Get msg */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)msg,
            &msg_len)) {
        return;
    }
    /* Get report */
    ISI_xdrGetInteger(xdr_ptr, (int*)&report);
    /* Get reportId */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reportId,
            &reportId_len)) {
        return;
    }
    ret = ISI_sendChatMessage(&msgId, chatId, msg, report, reportId);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put msgId */
    ISI_xdrPutInteger(xdr_ptr, msgId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverComposingChatMessage() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverComposingChatMessage(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             chatId;

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);

    ret = ISI_composingChatMessage(chatId);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSendChatMessageReport() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSendChatMessageReport(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             msgId;
    ISI_Id             chatId;
    ISI_MessageReport  report;
    char               reportId[ISI_ID_STRING_SZ + 1];
    int                reportId_len;

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);
    /* Get report */
    ISI_xdrGetInteger(xdr_ptr, (int*)&report);
    /* Get reportId */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)reportId,
            &reportId_len)) {
        return;
    }
    ret = ISI_sendChatMessageReport(&msgId, chatId, report, reportId);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put msgId */
    ISI_xdrPutInteger(xdr_ptr, msgId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSendChatFile() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSendChatFile(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             fileId;
    ISI_Id             chatId;
    char               subject[ISI_SUBJECT_STRING_SZ + 1];
    int                subject_len;
    char               filePath[ISI_FILE_PATH_STRING_SZ + 1];
    int                filePath_len;
    ISI_FileType       fileType;

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);
    /* Get subject */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)subject,
            &subject_len)) {
        return;
    }
    /* Get subject */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)filePath,
            &filePath_len)) {
        return;
    }
    /* Get fileType */
    ISI_xdrGetInteger(xdr_ptr, (int*)&fileType);
    ret = ISI_sendChatFile(&fileId, chatId, subject, filePath, fileType);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put fileId */
    ISI_xdrPutInteger(xdr_ptr, fileId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSendPrivateGroupChatMessage() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSendPrivateGroupChatMessage(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             msgId;
    ISI_Id             chatId;
    char               partipant[ISI_ADDRESS_STRING_SZ + 1];
    int                partipant_len;
    char               msg[ISI_TEXT_STRING_SZ + 1];
    int                msg_len;

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);
    /* Get partipant */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)partipant,
            &partipant_len)) {
        return;
    }
    /* Get msg */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)msg,
            &msg_len)) {
        return;
    }

    ret = ISI_sendPrivateGroupChatMessage(&msgId, chatId, partipant, msg);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put msgId */
    ISI_xdrPutInteger(xdr_ptr, msgId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSendGroupChatPresence() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSendGroupChatPresence(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             presId;
    ISI_Id             chatId;
    char               presence[ISI_PRESENCE_STRING_SZ + 1];
    int                presence_len;

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);
    /* Get presence */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)presence,
            &presence_len)) {
        return;
    }

    ret = ISI_sendGroupChatPresence(&presId, chatId, presence);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put presId */
    ISI_xdrPutInteger(xdr_ptr, presId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetChatState() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetChatState(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             chatId;
    ISI_ChatState      chatState;

    /* Get chatId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&chatId);

    ret = ISI_getChatState(chatId, &chatState);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put chatState */
    ISI_xdrPutInteger(xdr_ptr, chatState);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverMediaControl() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverMediaControl(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_MediaControl   cmd;
    int                arg;

    /* Get cmd */
    ISI_xdrGetInteger(xdr_ptr, (int*)&cmd);
    /* Get arg */
    ISI_xdrGetInteger(xdr_ptr, (int*)&arg);
    
    ret = ISI_mediaControl(cmd, arg);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceSetFilePath() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceSetFilePath(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             serviceId; 
    char               filePath[ISI_FILE_PATH_STRING_SZ + 1];
    int                filePath_len;
    char               filePrepend[ISI_ADDRESS_STRING_SZ + 1];
    int                filePrepend_len;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get filePath */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)filePath,
            &filePath_len)) {
        return;
    }
    /* Get filePrepend */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)filePrepend,
            &filePrepend_len)) {
        return;
    }
    ret = ISI_serviceSetFilePath(serviceId, filePath, filePrepend);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetServiceAtribute() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetServiceAtribute(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             evtId;
    ISI_Id             serviceId;
    ISI_SeviceAttribute cmd;
    char               arg1[ISI_SECURITY_KEY_STRING_SZ + 1];
    int                arg1_len;
    char               arg2[ISI_SECURITY_KEY_STRING_SZ + 1];
    int                arg2_len;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);
    /* Get cmd */
    ISI_xdrGetInteger(xdr_ptr, (int*)&cmd);
    /* Get filePath */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)arg1,
            &arg1_len)) {
        return;
    }
    /* Get filePrepend */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)arg2,
            &arg2_len)) {
        return;
    }
    ret = ISI_getServiceAtribute(&evtId, serviceId, cmd, arg1, arg2);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put ISI return evtId */
    ISI_xdrPutInteger(xdr_ptr, evtId);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSetAkaAuthResp() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSetAkaAuthResp(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return         ret;
    ISI_Id             serviceId;
    int                respResult;
    char               resp[ISI_AKA_AUTH_RESP_STRING_SZ];
    int                resp_len;
    char               auts[ISI_AKA_AUTH_AUTS_STRING_SZ];
    int                auts_len;
    char               ck[ISI_AKA_AUTH_CK_STRING_SZ];
    int                ck_len;
    char               ik[ISI_AKA_AUTH_IK_STRING_SZ];
    int                ik_len;
    int                resLength;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int *)&serviceId);
    /* Get aka response result */
    ISI_xdrGetInteger(xdr_ptr, &respResult);
    /* Get resp */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)resp,
            &resp_len)) {
        return;
    }
    /* Get the length of response */
    ISI_xdrGetInteger(xdr_ptr, &resLength);
    /* Get auts */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)auts,
            &auts_len)) {
        return;
    }
    /* Get ck */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)ck,
            &ck_len)) {
        return;
    }
    /* Get ik */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)ik,
            &ik_len)) {
        return;
    }
    ret = ISI_setAkaAuthResp(serviceId, respResult, resp, resLength, auts, ck,
            ik);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverGetAkaAuthChallenge() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverGetAkaAuthChallenge(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return           ret;
    ISI_Id               serviceId;
    char                 rand[ISI_SECURITY_KEY_STRING_SZ + 1];
    char                 autn[ISI_SECURITY_KEY_STRING_SZ + 1];

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);

    ret = ISI_getAkaAuthChallenge(serviceId, rand, autn);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);
    /* put rand */
    ISI_xdrPutString(xdr_ptr, rand, OSAL_strlen(rand) + 1);
    /* put autn */
    ISI_xdrPutString(xdr_ptr, autn, OSAL_strlen(autn) + 1);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceGetIpsec() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceGetIpsec(
    ISI_Xdr     *xdr_ptr)
{
    ISI_Return  ret;
    ISI_Id      serviceId;
    int         portUc, portUs, portPc, portPs;
    int         spiUc, spiUs, spiPc, spiPs;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);

    ret = ISI_serviceGetIpsec(serviceId,
            &portUc, &portUs, &portPc, &portPs,
            &spiUc, &spiUs, &spiPc, &spiPs);

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    ISI_xdrPutInteger(xdr_ptr, portUc);
    ISI_xdrPutInteger(xdr_ptr, portUs);
    ISI_xdrPutInteger(xdr_ptr, portPc);
    ISI_xdrPutInteger(xdr_ptr, portPs);
    ISI_xdrPutInteger(xdr_ptr, spiUc);
    ISI_xdrPutInteger(xdr_ptr, spiUs);
    ISI_xdrPutInteger(xdr_ptr, spiPc);
    ISI_xdrPutInteger(xdr_ptr, spiPs);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverDiagAudioRecord() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverDiagAudioRecord(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return           ret;
    char                 file[ISI_SECURITY_KEY_STRING_SZ + 1];
    vint                 file_len;

    /* Get file */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)file,
            &file_len)) {
        return;
    }
    ret = ISI_diagAudioRecord(file);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverDiagAudioPlay() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverDiagAudioPlay(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return           ret;
    char                 file[ISI_SECURITY_KEY_STRING_SZ + 1];
    vint                 file_len;

    /* Get file */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, (char *)file,
            &file_len)) {
        return;
    }
    ret = ISI_diagAudioPlay(file);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverServiceSetEmergency() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverServiceSetEmergency(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    int             isEmergency;

    /* Get serviceId */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);

    /* Get isEmergency */
    ISI_xdrGetInteger(xdr_ptr, (int*)&isEmergency);

    ret = ISI_serviceSetEmergency(serviceId, isEmergency);
    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverSetProvisioningData() ========
 * This function is used to map the ISI functions.
 * 
 * Return Values:
 *   None
 */
void _ISI_serverSetProvisioningData(
    ISI_Xdr   *xdr_ptr)
{
    ISI_Return      ret;
    ISI_Id          serviceId;
    char            xmlDoc[ISI_PROVISIONING_DATA_STRING_SZ];
    vint            xmlDocLen;
    ISI_ServerEvent isiEvt;

    /* Get call id */
    ISI_xdrGetInteger(xdr_ptr, (int*)&serviceId);

    /* Get xml doc */
    if (OSAL_FAIL == ISI_xdrGetString(xdr_ptr, xmlDoc,
            &xmlDocLen)) {
        ISI_rpcDbgPrintf("ISI_xdrGetString failed.\n");
        return;
    }

    /* Store provisioning data and send ISI server event for CSM */
    OSAL_strncpy(_ISI_ServerObj_ptr->service.provisioningData, xmlDoc,
            ISI_PROVISIONING_DATA_STRING_SZ);

    isiEvt.serviceId = serviceId;
    isiEvt.id = serviceId;
    isiEvt.type = ISI_ID_TYPE_SERVICE;
    isiEvt.evt = ISI_EVENT_RCS_PROVISIONING;
    isiEvt.eventDesc[0] = 0;
    if (OSAL_SUCCESS != OSAL_msgQSend(_ISI_ServerObj_ptr->csmEvtQ,
            &isiEvt, sizeof(ISI_ServerEvent), OSAL_NO_WAIT, NULL)) {
        ISI_rpcDbgPrintf("Failed to write ISI event to csm evt Q.\n");
    }
    
    
    ret = ISI_RETURN_OK;

    /* init xdr_ptr */
    ISI_xdrEncodeInit(xdr_ptr);
    /* put ISI return response */
    ISI_xdrPutInteger(xdr_ptr, ret);

    /* Write reponse backe to ISI client */
    if (OSAL_SUCCESS != _ISI_serverRpcWriteIsi(
            _ISI_ServerObj_ptr, xdr_ptr->data, 
            ISI_DATA_SIZE)) {
        ISI_rpcDbgPrintf("ISI Server write IsiRpc Failed\n");
    }
}

/*
 * ======== _ISI_serverProcessEvent() ========
 *
 *  process event loop for ISI_server 
 */
void _ISI_serverProcessEvt(
    ISI_ApiName isiApi,
    ISI_Xdr *xdr_ptr)
{
    switch (isiApi) {
        case ISI_INIT:
            _ISI_serverInit(xdr_ptr);
            break;
        case ISI_INITIATE_CALL:
            _ISI_serverInitiateCall(xdr_ptr);
            break;
        case ISI_GET_NEXT_SERVICE:
            _ISI_serverGetNextService(xdr_ptr);
            break;          
        case ISI_GET_VERSION:
            _ISI_serverGetVersion(xdr_ptr);
            break;
        case ISI_SHUTDOWN:
            _ISI_serverShutdown(xdr_ptr);
            break;
        case ISI_ALLOC_SERVICE:
            _ISI_serverAllocService(xdr_ptr);
            break;
        case ISI_ACTIVATE_SERVICE:
            _ISI_serverActivateService(xdr_ptr);
            break;
        case ISI_DEACTIVATE_SERVICE:
            _ISI_serverDeactivateService(xdr_ptr);
            break;
        case ISI_FREE_SERVICE:
            _ISI_serverFreeService(xdr_ptr);
            break;
        case ISI_SERVICE_MAKE_CID_PRIVATE:
            _ISI_serverServiceMakeCidPrivate(xdr_ptr);
            break;
        case ISI_SERVICE_SET_BSID:
            _ISI_serverServiceSetBsid(xdr_ptr);
            break;
        case ISI_SERVICE_SET_IMEI_URI:
            _ISI_serverServiceSetImeiUri(xdr_ptr);
            break;
        case ISI_SERVICE_SET_INTERFACE:
            _ISI_serverServiceSetInterface(xdr_ptr);
            break;
        case ISI_SERVICE_SET_URI:
            _ISI_serverServiceSetUri(xdr_ptr);
            break;
        case ISI_SERVICE_SET_CAPABILITIES:
            _ISI_serverServiceSetCapabilities(xdr_ptr);
            break;
        case ISI_SERVICE_SET_SERVER:
            _ISI_serverServiceSetServer(xdr_ptr);
            break;
        case ISI_SERVICE_SET_PORT:
            _ISI_serverServiceSetPort(xdr_ptr);
            break;
        case ISI_SERVICE_SET_CREDENTIALS:
            _ISI_serverServiceSetCredentials(xdr_ptr);
            break;
        case ISI_SERVICE_SET_IPSEC:
            _ISI_serverServiceSetIpsec(xdr_ptr);
            break;
        case ISI_SERVICE_SET_DEVICEID:
            _ISI_serverServiceSetDeviceId(xdr_ptr);
            break;
        case ISI_SERVICE_BLOCK_USER:
            _ISI_serverServiceBlockUser(xdr_ptr);
            break;
        case ISI_SERVICE_UNBLOCK_USER:
            _ISI_serverServiceUnblockUser(xdr_ptr);
            break;
        case ISI_ADD_CODER_TO_SERVICE:
            _ISI_serverAddCoderToService(xdr_ptr);
            break;
        case ISI_REMOVE_CODER_FROM_SERVICE:
            _ISI_serverRemoveCoderFromService(xdr_ptr);
            break;
        case ISI_SERVICE_FORWARD_CALLS:
            _ISI_serverServiceForwardCalls(xdr_ptr);
            break;
        case ISI_SEND_TELEVENT_TO_REMOTE:
            _ISI_serverSendTelEventToRemote(xdr_ptr);
            break;
        case ISI_SEND_TELEVENT_STRING_TO_REMOTE:
            _ISI_serverSendTelEventStringToRemote(xdr_ptr);
            break;
        case ISI_GET_TELEVENT_FROM_REMOTE:
            _ISI_serverGetTelEventFromRemote(xdr_ptr);
            break;
        case ISI_GET_TELEVENT_RESPONSE:
            _ISI_serverGetTelEventResponse(xdr_ptr);
            break;
        case ISI_SEND_USSD:
            _ISI_serverSendUSSD(xdr_ptr);
            break;
        case ISI_TERMINATE_CALL:
            _ISI_serverTerminateCall(xdr_ptr);
            break;
        case ISI_MODIFY_CALL:
            _ISI_serverModifyCall(xdr_ptr);
            break;
        case ISI_ACCEPT_CALL_MODIFY:
            _ISI_serverAcceptCallModify(xdr_ptr);
            break;
        case ISI_REJECT_CALL_MODIFY:
            _ISI_serverRejectCallModify(xdr_ptr);
            break;
        case ISI_GET_CALL_STATE:
            _ISI_serverGetCallState(xdr_ptr);
            break;
        case ISI_ADD_CODER_TO_CALL:
            _ISI_serverAddCoderToCall(xdr_ptr);
            break;
        case ISI_REMOVE_CODER_FROM_CALL:
            _ISI_serverRemoveCoderFromCall(xdr_ptr);
            break;
        case ISI_SET_CALL_SESSION_DIRECTION:
            _ISI_serverSetCallSessionDirection(xdr_ptr);
            break;
        case ISI_GET_CALL_SESSION_DIRECTION:
            _ISI_serverGetCallSessionDirection(xdr_ptr);
            break;
        case ISI_GET_CALL_HEADER:
            _ISI_serverGetCallHeader(xdr_ptr);
            break;
        case ISI_GET_SUPSRV_HEADER:
            _ISI_serverGetSupsrvHeader(xdr_ptr);
            break;    
        case ISI_GET_CALL_SESSION_TYPE:
            _ISI_serverGetCallSessionType(xdr_ptr);
            break;
        case ISI_GET_CODER_DESCRIPTION:
            _ISI_serverGetCoderDescription(xdr_ptr);
            break;
        case ISI_ACKNOWLEDGE_CALL:
            _ISI_serverAcknowledgeCall(xdr_ptr);
            break;
        case ISI_HOLD_CALL:
            _ISI_serverHoldCall(xdr_ptr);
            break;
        case ISI_RESUME_CALL:
            _ISI_serverResumeCall(xdr_ptr);
            break;
        case ISI_ACCEPT_CALL:
             _ISI_serverAcceptCall(xdr_ptr);
            break;
        case ISI_REJECT_CALL:
            _ISI_serverRejectCall(xdr_ptr);
            break;
        case ISI_FORWARD_CALL:
            _ISI_serverForwardCall(xdr_ptr);
            break;
        case ISI_BLIND_TRANSFER_CALL:
            _ISI_serverBlindTransferCall(xdr_ptr);
            break;
        case ISI_ATTENDED_TRANSFER_CALL:
            _ISI_serverAttendedTransferCall(xdr_ptr);
            break;
        case ISI_CONSULTATIVE_TRANSFER_CALL:
            _ISI_serverConsultativeTransferCall(xdr_ptr);
            break;
        case ISI_GENERATE_TONE:
            _ISI_serverGenerateTone(xdr_ptr);
            break;
        case ISI_STOP_TONE:
            _ISI_serverStopTone(xdr_ptr);
            break;
        case ISI_START_CONF_CALL:
            _ISI_serverStartConfCall(xdr_ptr);
            break;               
        case ISI_ADD_CALL_TO_CONF:
            _ISI_serverAddCallToConf(xdr_ptr);
            break;
        case ISI_REMOVE_CALL_FROM_CONF:
            _ISI_serverRemoveCallFromConf(xdr_ptr);
            break;
        case ISI_SEND_MESSAGE:
            _ISI_serverSendMessage(xdr_ptr);
            break;
        case ISI_SEND_MESSAGE_REPORT:
            _ISI_serverSendMessageReport(xdr_ptr);
            break;
        case ISI_READ_MESSAGE_REPORT:
            _ISI_serverReadMessageReport(xdr_ptr);
            break;
        case ISI_GET_MESSAGE_HEADER:
            _ISI_serverGetMessageHeader(xdr_ptr);
            break;
        case ISI_READ_MESSAGE:
            _ISI_serverReadMessage(xdr_ptr);
            break;
        case ISI_SEND_FILE:
            _ISI_serverSendFile(xdr_ptr);
            break;
        case ISI_ACCEPT_FILE:
            _ISI_serverAcceptFile(xdr_ptr);
            break;
        case ISI_ACKNOWLEDGE_FILE:
            _ISI_serverAcknowledgeFile(xdr_ptr);
            break;
        case ISI_REJECT_FILE:
            _ISI_serverRejectFile(xdr_ptr);
            break;
        case ISI_CANCEL_FILE:
            _ISI_serverCancelFile(xdr_ptr);
            break;                
        case ISI_GET_FILE_HEADER:
            _ISI_serverGetFileHeader(xdr_ptr);
            break;
        case ISI_READ_FILE_PROGRESS:
            _ISI_serverReadFileProgress(xdr_ptr);
            break;
        case ISI_ADD_CONTACT:
            _ISI_serverAddContact(xdr_ptr);
            break;
        case ISI_REMOVE_CONTACT:
            _ISI_serverRemoveContact(xdr_ptr);
            break;
        case ISI_READ_CONTACT:
            _ISI_serverReadContact(xdr_ptr);
            break;
        case ISI_SUBSCRIBE_TO_PRESENCE:
            _ISI_serverSubscribeToPresence(xdr_ptr);
            break;
        case ISI_UNSUBSCRIBE_FROM_PRESENCE:
            _ISI_serverUnsubscribeFromPresence(xdr_ptr);
            break;
        case ISI_READ_SUBSCRIBE_TO_PRESENCE_REQUEST:
            _ISI_serverReadSubscribeToPresenceRequest(xdr_ptr);
            break;
        case ISI_READ_SUBSCRIPTION_TO_PRESENCE_RESPONSE:
            _ISI_serverReadSubscriptionToPresenceResponse(xdr_ptr);
            break;
        case ISI_ALLOW_SUBSCRIPTION_TO_PRESENCE:
            _ISI_serverAllowSubscriptionToPresence(xdr_ptr);
            break;
        case ISI_DENY_SUBSCRIPTION_TO_PRESENCE:
            _ISI_serverDenySubscriptionToPresence(xdr_ptr);
            break;
        case ISI_SEND_PRESENCE:
            _ISI_serverSendPresence(xdr_ptr);
            break;
        case ISI_READ_PRESENCE:
            _ISI_serverReadPresence(xdr_ptr);
            break;
        case ISI_SEND_CAPABILITIES:
            _ISI_serverSendCapabilities(xdr_ptr);
            break;
        case ISI_READ_CAPABILITIES:
            _ISI_serverReadCapabilities(xdr_ptr);
            break;
        case ISI_GET_CALL_HANDOFF:
            _ISI_serverGetCallHandoff(xdr_ptr);
            break;
        case ISI_SET_CHAT_NICKNAME:
            _ISI_serverSetChatNickname(xdr_ptr);
            break;
        case ISI_INITIATE_GROUP_CHAT:
            _ISI_serverInitiateGroupChat(xdr_ptr);
            break;
        case ISI_INITIATE_GROUP_CHAT_ADHOC:
            _ISI_serverInitiateGroupChatAdhoc(xdr_ptr);
            break;
        case ISI_INITIATE_CHAT:
            _ISI_serverInitiateChat(xdr_ptr);
            break;
        case ISI_ACCEPT_CHAT:
            _ISI_serverAcceptChat(xdr_ptr);
            break;
        case ISI_REJECT_CHAT:
            _ISI_serverRejectChat(xdr_ptr);
            break;
        case ISI_ACKNOWLEDGE_CHAT:
            _ISI_serverAcknowledgeChat(xdr_ptr);
            break;
        case ISI_DISCONNECT_CHAT:
            _ISI_serverDisconnectChat(xdr_ptr);
            break;
        case ISI_GET_CHAT_HEADER:
            _ISI_serverGetChatHeader(xdr_ptr);
            break;
        case ISI_GET_GROUP_CHAT_HEADER:
            _ISI_serverGetGroupChatHeader(xdr_ptr);
            break;
        case ISI_READ_GROUP_CHAT_PRESENCE:
            _ISI_serverReadGroupChatPresence(xdr_ptr);
            break;
        case ISI_INVITE_GROUP_CHAT:
            _ISI_serverInviteGroupChat(xdr_ptr);
            break;
        case ISI_JOIN_GROUP_CHAT:
            _ISI_serverJoinGroupChat(xdr_ptr);
            break;
        case ISI_KICK_GROUP_CHAT:
            _ISI_serverKickGroupChat(xdr_ptr);
            break;
        case ISI_DESTROY_GROUP_CHAT:
            _ISI_serverDestroyGroupChat(xdr_ptr);
            break;
        case ISI_SEND_CHAT_MESSAGE:
            _ISI_serverSendChatMessage(xdr_ptr);
            break;
        case ISI_COMPOSING_CHAT_MESSAGE:
            _ISI_serverComposingChatMessage(xdr_ptr);
            break;
        case ISI_SEND_CHAT_MESSAGE_REPORT:
            _ISI_serverSendChatMessageReport(xdr_ptr);
            break;
        case ISI_SEND_CHAT_FILE:
            _ISI_serverSendChatFile(xdr_ptr);
            break;
        case ISI_SEND_PRIVATE_GROUP_CHAT_MESSAGE:
            _ISI_serverSendPrivateGroupChatMessage(xdr_ptr);
            break;
        case ISI_SEND_GROUP_CHAT_PRESENCE:
            _ISI_serverSendGroupChatPresence(xdr_ptr);
            break;
        case ISI_GET_CHAT_STATE:
            _ISI_serverGetChatState(xdr_ptr);
            break;
        case ISI_MEDIA_CONTROL:
            _ISI_serverMediaControl(xdr_ptr);
            break;
        case ISI_SERVICE_SET_FILE_PATH:
            _ISI_serverServiceSetFilePath(xdr_ptr);
            break;
        case ISI_GET_SERVICE_ATRIBUTE:
            _ISI_serverGetServiceAtribute(xdr_ptr);
            break;
        case ISI_SET_AKA_AUTH_RESP:
            _ISI_serverSetAkaAuthResp(xdr_ptr);
            break;
        case ISI_GET_AKA_AUTH_CHALLENGE:
            _ISI_serverGetAkaAuthChallenge(xdr_ptr);
            break;
        case ISI_SERVICE_GET_IPSEC:
            _ISI_serverServiceGetIpsec(xdr_ptr);
            break;
        case ISI_DIAG_AUDIO_RECORD:
            _ISI_serverDiagAudioRecord(xdr_ptr);
            break;
        case ISI_DIAG_AUDIO_PLAY:
            _ISI_serverDiagAudioPlay(xdr_ptr);
            break;
        case ISI_SERVICE_SET_EMERGENCY:
            _ISI_serverServiceSetEmergency(xdr_ptr);
            break;
        case ISI_SET_FEATURE:
            _ISI_serverSetFeature(xdr_ptr);
            break;
        case ISI_SERVICE_SET_FEATURE:
            _ISI_serverServiceSetFeature(xdr_ptr);
            break;
        case ISI_SERVICE_GET_FEATURE:
            _ISI_serverServiceGetFeature(xdr_ptr);
            break;
        case ISI_SET_CALL_RESOURCE_STATUS:
            _ISI_serverSetCallResourceStatus(xdr_ptr);
            break;
        case ISI_GET_CALL_RESOURCE_STATUS:
            _ISI_serverGetCallResourceStatus(xdr_ptr);
            break;
        case ISI_GET_CALL_SRVCC_STATUS:
            _ISI_serverGetCallSrvccStatus(xdr_ptr);
            break;
        case ISI_READ_USSD:
            /* Not implemented yet */
            break;
        case ISI_SET_PROVISIONING_DATA:
            _ISI_serverSetProvisioningData(xdr_ptr);
            break;
        case ISI_UPDATE_CALL_SESSION:
            _ISI_serverUpdateCallSession(xdr_ptr);
        default:
            break;
    }

}

/*
 * ======== _ISI_serverDaemon() ========
 *
 * Main processing loop for ISI_server 
 */
int32 _ISI_serverDaemon(
    void *arg_ptr)
{
    ISI_Xdr    *xdr_ptr;
    ISI_ApiName isiApi;

    xdr_ptr = &_ISI_ServerObj_ptr->xdr;

 _ISI_SERVER_DAEMON_LOOP:
    if (OSAL_FAIL == ISI_xdrDecodeInit(xdr_ptr)) {
        return (-1);
    }
    
    if (OSAL_SUCCESS == _ISI_serverRpcReadIsi(
            _ISI_ServerObj_ptr, &xdr_ptr->data, 
            ISI_DATA_SIZE,
            OSAL_WAIT_FOREVER)) {
        if (OSAL_FAIL == ISI_xdrGetInteger(xdr_ptr, (int*)&isiApi)) {
            goto _ISI_SERVER_DAEMON_LOOP;
        }
        _ISI_serverProcessEvt(isiApi,xdr_ptr);
    }
    else {
        /* Then there was an error. The client died! Sleep for a little */
        OSAL_taskDelay(2000);
    }
    goto _ISI_SERVER_DAEMON_LOOP;
}

/*
 * ======== _ISI_serverDaemon() ========
 *
 * This function is used to receive function call, ISI_getEvent(), from VPAD.
 * This function will receive ISI event from ISI and send it back to VPAD.
 */
int32 _ISI_serverGetEvtTask(
    void *arg_ptr)
{
    ISI_Xdr     xdr;
    ISI_ApiName isiApi;

_ISI_SERVER_GET_EVT_TASK_LOOP:
    if (OSAL_FAIL == ISI_xdrDecodeInit(&xdr)) {
        return (-1);
    }

    if (OSAL_SUCCESS == _ISI_serverRpcReadIsiEvt(
            _ISI_ServerObj_ptr, xdr.data, 
            ISI_DATA_SIZE, OSAL_WAIT_FOREVER)) {
        if (OSAL_FAIL == ISI_xdrGetInteger(&xdr, (int*)&isiApi)) {
            goto _ISI_SERVER_GET_EVT_TASK_LOOP;
        }

        if (ISI_GET_EVENT == isiApi) {
            _ISI_serverGetEvent(&xdr);
        }
    }

    goto _ISI_SERVER_GET_EVT_TASK_LOOP;
}

/*
 * ======== _ISI_startServerTask() ========
 *
 * This function is used to create a main loop to process ISI_server  .
 *
 */
OSAL_Status _ISI_startServerTask(void)
{
    if (0 == (_ISI_ServerObj_ptr->taskId = OSAL_taskCreate( 
            _ISI_SERVER_TASK_NAME, 
            OSAL_TASK_PRIO_NRT, 
            _ISI_SERVER_TASK_STACK_BYTES, 
            _ISI_serverDaemon, 
            NULL))) { 
        ISI_rpcDbgPrintf("Failed creating %s daemon\n", _ISI_SERVER_TASK_NAME); 
        return (OSAL_FAIL); 
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _ISI_startEventTask() ========
 *
 * This function is used to create a event loop to receive ISI event from ISI and send it back to VPAD  .
 *
 */
OSAL_Status _ISI_startEventTask(void)
{
    if (0 == (_ISI_ServerObj_ptr->evtTaskId = OSAL_taskCreate( 
            _ISI_SERVER_EVT_TASK_NAME, 
            OSAL_TASK_PRIO_NRT, 
            _ISI_SERVER_TASK_STACK_BYTES, 
            _ISI_serverGetEvtTask, 
            0))) { 
        ISI_rpcDbgPrintf("Failed creating %s daemon\n", _ISI_SERVER_EVT_TASK_NAME); 
        return (OSAL_FAIL); 
    }
    return (OSAL_SUCCESS);
}
