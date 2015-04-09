/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#include "_rpm.h"
#include "_rpm_utils.h"

/* Global RPM Object pointer */
_RPM_Obj *_RPM_objPtr;

/*
 * ======== _RPM_getObject() ========
 *
 * This function returns a pointer to the RPM object singleton.
 *
 * WARNING this must be called after RPM_init() exits
 *
 * Returns:
 *  A pointer to the _RPM_Obj singleton
 *
 */
_RPM_Obj* _RPM_getObject(
    void)
{
    return (_RPM_objPtr);
}

/*
 * ======== _RPM_shouldUseIp() ========
 *
 * This function is to chech IP service should be used.
 *
 * Returns:
 *  OSAL_TRUE: Should use ip service.
 *  OSAL_FALSE: Should not use ip service.
 *
 */
OSAL_Boolean _RPM_shouldUseIp(
    RPM_FeatureType type)
{
    RPM_RadioInterface *radio_ptr;
    RFSM_Context       *rfsmContext_ptr;

    RPM_dbgPrintf("\n");

    rfsmContext_ptr = &_RPM_objPtr->radioFsm;

    if (RPM_FEATURE_TYPE_CALL_CS_EMERGENCY == type) {
        /* Always false for Cs emergency call */
        return (OSAL_FALSE);
    }

    /* Get radio interface by feature type */
    if ((RPM_FEATURE_TYPE_CALL_EMERGENCY == type) ||
            (RPM_FEATURE_TYPE_IMS_EMERGENCY_SERVICE == type)) {
        radio_ptr = &rfsmContext_ptr->emergencyRadioInfc;
    }
    else {
        radio_ptr = &rfsmContext_ptr->radioInfc;
    }

    /* If radio type is lte or wifi and its ip isn't 0 then return true */
    if ((RPM_RADIO_TYPE_LTE == radio_ptr->radioType) &&
            (OSAL_FALSE == 
            OSAL_netIsAddrZero(&rfsmContext_ptr->lteRadioInfc.ipAddr))) {
        return (OSAL_TRUE);
    }
    else if ((RPM_RADIO_TYPE_LTE_EMERGENCY == radio_ptr->radioType) &&
             (OSAL_FALSE == 
             OSAL_netIsAddrZero(&rfsmContext_ptr->emergencyRadioInfc.ipAddr))) {
        return (OSAL_TRUE);
    }
    else if ((RPM_RADIO_TYPE_WIFI == radio_ptr->radioType) &&
            (OSAL_FALSE == 
            OSAL_netIsAddrZero(&rfsmContext_ptr->wifiRadioInfc.ipAddr))) {
        return (OSAL_TRUE);
    }
    else {
        return (OSAL_FALSE);
    }
}

/*
 * ======== _RPM_shouldUpdateIpAddress() ========
 *
 * Private function to check if RPM should call imsChangeCb to update the ip
 * address changes according the ip change event RPM received.
 *
 * Returns:
 *  OSAL_TRUE: Should update the ip address change.
 *  OSAL_FALSE: Should not update the ip address change.
 *
 */
OSAL_Boolean _RPM_shouldUpdateIpAddress(
    RPM_RadioInterface *radio_ptr,
    RPM_Event *event_ptr)
{
    RPM_dbgPrintf("\n");

    /* supsrv ip event won't need to update ims ip */
    if (RPM_RADIO_TYPE_LTE_SS == event_ptr->radioType) {
        return (OSAL_FALSE);
    }

    /*
     * Check current RPM state if need to notify IP change.
     */
    if (RPM_RADIO_TYPE_WIFI == radio_ptr->radioType) {
        /*
         * RPM does not need to notify lte ip change in wifi node.
         */
        if (RPM_RADIO_TYPE_LTE == event_ptr->radioType) {
            return (OSAL_FALSE);
        }
    }
    else if (RPM_RADIO_TYPE_LTE == radio_ptr->radioType) {
        /*
         * RPM does not need to notify ip change,
         * if wifi losts ip address in lte mode.
         */
        if ((OSAL_netIsAddrZero(&event_ptr->u.ipAddress)) &&
                (RPM_RADIO_TYPE_WIFI == event_ptr->radioType)) {
            return (OSAL_FALSE);
        }
    }

    /* Otherwise return OSAL_TRUE */
    return (OSAL_TRUE);
}

/*
 * ======== RPM_init() ========
 *
 * This function initializes the RPM package
 *
 * Returns:
 *     RPM_RETURN_OK: if no issues
 *     RPM_RETURN_FAIL: otherwise
 */
RPM_Return RPM_init(
    void)
{
    RPM_dbgPrintf("\n");

    /*
     * Init and clear the global object
     */
    _RPM_objPtr = OSAL_memCalloc(1, sizeof(_RPM_Obj), 0);
    OSAL_memSet(_RPM_objPtr, 0, sizeof(_RPM_Obj));

    /*
     * Init the radio FSM
     */
    RFSM_init(&_RPM_objPtr->radioFsm); 
    return (RPM_RETURN_OK);
}

/*
 * ======== RPM_shutdown() ========
 *
 * This cleans up and shutsdown the RPM package
 *
 * Returns:
 *     none
 */
void RPM_shutdown(
    void)
{
    RPM_dbgPrintf("\n");

    OSAL_memFree(_RPM_objPtr, 0);
}

/*
 * ======== RPM_processEvent() ========
 *
 * This function passes an event into the RPM library and it's state machines
 *
 * Returns:
 *     RPM_RETURN_OK: if event processed successfully
 *     RPM_RETURN_FAIL: otherwise
 */
RPM_Return RPM_processEvent(
    RPM_Event *event_ptr)
{
    RPM_RadioInterface *radio_ptr;

    RPM_dbgPrintf("\n");
    switch (event_ptr->type) {
        case RPM_EVENT_TYPE_EMERGENCY_CALL:
            break;
        case RPM_EVENT_TYPE_SRVCC_START:
        case RPM_EVENT_TYPE_SRVCC_FAIL:
            /* Call registered callback and pass the event type */
            if (_RPM_objPtr->srvccChangeCb) {
                _RPM_objPtr->srvccChangeCb(event_ptr->type);
            }
            break;
        case RPM_EVENT_TYPE_IP_CHANGE:
            /* Get radio interface */
            if (RPM_RADIO_TYPE_LTE_EMERGENCY == event_ptr->radioType) {
                /* Get emergency radio interface */
                radio_ptr = &_RPM_objPtr->radioFsm.emergencyRadioInfc;
                radio_ptr->ipAddr = event_ptr->u.ipAddress;
            }
            else if (RPM_RADIO_TYPE_WIFI == event_ptr->radioType) {
                /* Store the wifi ip address. */
                radio_ptr = &_RPM_objPtr->radioFsm.wifiRadioInfc;
                radio_ptr->radioType = event_ptr->radioType;
                radio_ptr->ipAddr = event_ptr->u.ipAddress;
                RPM_dbgPrintf("Got wifi ip %d\n", event_ptr->u.ipAddress.ipv4);
            }
            else if (RPM_RADIO_TYPE_LTE == event_ptr->radioType) {
                /* Store the lte ip address. */
                radio_ptr = &_RPM_objPtr->radioFsm.lteRadioInfc;
                radio_ptr->radioType = event_ptr->radioType;
                radio_ptr->ipAddr = event_ptr->u.ipAddress;
                RPM_dbgPrintf("Got lte ip %d\n", event_ptr->u.ipAddress.ipv4);
            }
            else if (RPM_RADIO_TYPE_LTE_SS == event_ptr->radioType) {
                /* Store the lte supsrv ip address. */
                radio_ptr = &_RPM_objPtr->radioFsm.lteSupsrvRadioInfc;
                radio_ptr->radioType = event_ptr->radioType;
                radio_ptr->ipAddr = event_ptr->u.ipAddress;
                RPM_dbgPrintf("Got lte supsrv ip %d\n", event_ptr->u.ipAddress.ipv4);
            }
            else {
                RPM_dbgPrintf("Unknown radio type %d\n", event_ptr->radioType);
                break;
            }

            /*
             * Determine if we should update ip address according to current
             * radio state and ip change event.
             */
            if ((OSAL_TRUE == _RPM_shouldUpdateIpAddress(
                    &_RPM_objPtr->radioFsm.radioInfc, event_ptr)) &&
                    (NULL != _RPM_objPtr->imsChangeCb)) {
                /* Get available radio for updating */
                /* xxx supsrv radio defined(PROVIDER_CMCC) */
                if (RPM_RADIO_TYPE_LTE_EMERGENCY != event_ptr->radioType) {
                    RPM_getAvailableRadio(RPM_FEATURE_TYPE_IMS_SERVICE,
                            radio_ptr);
                    /* Notify listeners we have active radio */
                    _RPM_objPtr->imsChangeCb(radio_ptr, radio_ptr->radioType);
#if (!defined(PROVIDER_CMCC))
                    /* 
                     * default supsrv radio would be the same as ims radio
                     * add provider to the #if to exclude if that provider, like CMCC,
                     * would like different supsrv APN with IMS APN
                     */
                    if (NULL != _RPM_objPtr->supsrvChangeCb) {
                        _RPM_objPtr->supsrvChangeCb(radio_ptr, event_ptr->radioType);
                    }
#endif
                }
                else {
                    /* Notify listeners we have active emergency radio */
                    _RPM_objPtr->imsChangeCb(radio_ptr, event_ptr->radioType);
                }
            }
            if ((RPM_RADIO_TYPE_LTE_SS == radio_ptr->radioType) &&
                    (NULL != _RPM_objPtr->supsrvChangeCb)) {
                _RPM_objPtr->supsrvChangeCb(radio_ptr, event_ptr->radioType);
            }
            break;
        case RPM_EVENT_TYPE_SERVICE_STATE_CHANGE:
            RFSM_processEvent(&_RPM_objPtr->radioFsm, event_ptr);
            break;
        case RPM_EVENT_TYPE_SERVICE_CHANGE:
            /* Service configurations */
            if (RPM_RADIO_TYPE_LTE_EMERGENCY == event_ptr->radioType) {
                /* Set configurations to RPM object */
                _RPM_objPtr->isEmergencyFailoverToCs =
                        event_ptr->u.config.isEmergencyFailoverToCs;
                if ((_RPM_objPtr->isEmergencyRegRequired !=
                        event_ptr->u.config.isEmergencyRegRequired) &&
                        (NULL != _RPM_objPtr->eRegRequiredCb)) {
                    _RPM_objPtr->isEmergencyRegRequired =
                            event_ptr->u.config.isEmergencyRegRequired;                        
                    _RPM_objPtr->eRegRequiredCb(
                        event_ptr->u.config.isEmergencyRegRequired);
                }
            }
            break;
        default:
            OSAL_logMsg("%s:%d ERROR bad event type", __FUNCTION__, __LINE__);
            return (RPM_RETURN_ERROR);
    }
    return (RPM_RETURN_OK);
}

/*
 * ======== RPM_setServiceState() ========
 *
 * This function allows external users of to signal RPM when the state of a
 * given RPM_ServiceType changes
 *
 * Returns:
 *     none
 */
void RPM_setServiceState(
    RPM_RadioType    radioType,
    RPM_ServiceState serviceState)
{
    RPM_Event event;

    RPM_dbgPrintf("\n");
    /*  This is a convenience method.  Convert to an RPM_Event */
    event.type         = RPM_EVENT_TYPE_SERVICE_STATE_CHANGE;
    event.radioType    = radioType;
    event.u.serviceState = serviceState;

    /* Pump the Radio FSM */
    RPM_processEvent(&event);
}

/*
 * ======== RPM_registerImsRadioChangeCallback() ========
 *
 * This function allows external users of RPM to register a callback function
 * in order to receive notification when a radio change occurs for the IMS
 * service
 *
 * Returns:
 *     none
 *
 */
void RPM_registerImsRadioChangeCallback(
    RPM_notifyImsRadioChange imsCb)
{
    RPM_dbgPrintf("\n");
    _RPM_objPtr->imsChangeCb = imsCb;
}

/*
 * ======== RPM_registerSupsrvRadioChangeCallback() ========
 *
 * This function allows external users of RPM to register a callback function
 * in order to receive notification when a radio change occurs for the supsrv
 * service
 *
 * Returns:
 *     none
 *
 */
void RPM_registerSupsrvRadioChangeCallback(
    RPM_notifySupsrvChange supsrvCb)
{
    RPM_dbgPrintf("\n");
    _RPM_objPtr->supsrvChangeCb = supsrvCb;
}

/*
 * ======== RPM_registerSrvccChangeCallback() ========
 *
 * This function allows external users of RPM to register a callback function
 * in order to receive notification when an SRVCC occurs
 *
 * Returns:
 *     none
 *
 */
void RPM_registerSrvccChangeCallback(
    RPM_notifySrvccChange srvccCb)
{
    RPM_dbgPrintf("\n");
    _RPM_objPtr->srvccChangeCb = srvccCb;
}

/*
 * ======== RPM_eRegisterRequiredChangeCallback() ========
 *
 * This function allows external users of RPM to register a callback function
 * in order to receive notification when the emergency required configureation
 * changed.
 *
 * Returns:
 *     none
 *
 */
void RPM_eRegisterRequiredChangeCallback(
    RPM_notifyEmerRegChange emerRegCb)
{
    RPM_dbgPrintf("\n");
    _RPM_objPtr->eRegRequiredCb = emerRegCb;    
}

/*
 * ======== RPM_normalizeInboundAddress() ========
 *
 * This function allows external users normalize an inbound string address
 * via the Radio Policy defined in RPM
 *
 * Returns:
 *     RPM_RETURN_OK: if normalization was succesfull
 *     RPM_RETURN_FAIL: otherwise
 */
RPM_Return RPM_normalizeInboundAddress(
    RPM_RadioType  type,
    const char    *domain_ptr,
    const char    *address_ptr,
    char          *out_ptr,
    vint           maxOutLen,
    char          *display_ptr,
    vint           maxDisplayLen)
{
    RPM_Return ret;

    RPM_dbgPrintf("domain:%s address:%s display:%s \n", domain_ptr,
            address_ptr, display_ptr);

    /* Check if we are using CS or IP */
    if (RPM_RADIO_TYPE_LTE == type) {
        ret = RPM_utilInBoundIpNormalize(domain_ptr, address_ptr, out_ptr,
                maxOutLen, display_ptr, maxDisplayLen);
    }
    else {
        ret = RPM_utilInBoundCsNormalize(address_ptr, out_ptr, maxOutLen);
    }
    return (ret);
}

/*
 * ======== RPM_normalizeOutboundAddress() ========
 *
 * This function allows external users normalize an outbound string address
 * via the Radio Policy defined in RPM
 *
 * Returns:
 *     RPM_RETURN_OK: if normalization was succesfull
 *     RPM_RETURN_FAIL: otherwise
 */
RPM_Return RPM_normalizeOutboundAddress(
    const char      *domain_ptr,
    const char     *address_ptr,
    char           *out_ptr,
    vint            maxOutLen,
    RPM_FeatureType type)
{
    RPM_Return ret;

    RPM_dbgPrintf("\n");

    /* Check if we are using CS or IP */
    if (OSAL_TRUE == _RPM_shouldUseIp(type)) {
        ret = RPM_utilOutBoundIpNormalize(domain_ptr, address_ptr, out_ptr,
                maxOutLen);
    }
    else {
        ret = RPM_utilOutBoundCsNormalize(domain_ptr, address_ptr, out_ptr,
                maxOutLen);
    }
    return (ret);
}

/*
 * ======== RPM_normalizeOutboundAddress() ========
 *
 * This function is used to get the name or phone number from address.
 *
 * Returns:
 *     RPM_RETURN_OK: successful
 *     RPM_RETURN_FAIL: fail
 */
RPM_Return RPM_getNameFromAddress(
    const char *address_ptr,
    char       *out_ptr,
    vint        maxOutSize)
{
    return (RPM_utilGetNameFromAddress(address_ptr, out_ptr, maxOutSize));
}

/*
 * ======== RPM_getActiveRadio() ========
 *
 * This function allows external query of the current RadioInterface for a
 * given RPM_FeatureType. Noted that supsrv feature is not allowed here.
 *
 * Arguments:
 *     type: user requested RPM_FeatureType for which RPM_RadioInterface
 *            will be returned.
 *     *radioInfc: caller supplied object to be filled out by the routine.
 *
 * Returns:
 *     RPM_RETURN_SUCCES if no issue
 *     RPM_RETURN_ERROR: otherwise
 */
RPM_Return RPM_getActiveRadio(
    RPM_FeatureType type,
    RPM_RadioInterface *radioInfc_ptr)
{
    RFSM_Context *rfsmContext_ptr;
    RPM_RadioInterface *radio_ptr;

    RPM_dbgPrintf("\n");

    rfsmContext_ptr = &_RPM_objPtr->radioFsm;

    if ((RPM_FEATURE_TYPE_CALL_EMERGENCY == type) ||
            (RPM_FEATURE_TYPE_IMS_EMERGENCY_SERVICE == type)) {
        radio_ptr = &rfsmContext_ptr->emergencyRadioInfc;
    }
    else {
        radio_ptr = &rfsmContext_ptr->radioInfc;
    }

    radioInfc_ptr->radioType  = radio_ptr->radioType;
    if (RPM_RADIO_TYPE_LTE == radioInfc_ptr->radioType) {
        radioInfc_ptr->ipAddr  = rfsmContext_ptr->lteRadioInfc.ipAddr;
    }
    else if (RPM_RADIO_TYPE_LTE_EMERGENCY == radioInfc_ptr->radioType) {
        radioInfc_ptr->ipAddr  = rfsmContext_ptr->emergencyRadioInfc.ipAddr;
    }
    else if (RPM_RADIO_TYPE_WIFI == radioInfc_ptr->radioType) {
        radioInfc_ptr->ipAddr  = rfsmContext_ptr->wifiRadioInfc.ipAddr;
    }
    else {
        radioInfc_ptr->ipAddr  = radio_ptr->ipAddr;
    }

    return (RPM_RETURN_OK);
}

/*
 * ======== RPM_getAvailableRadio() ========
 *
 * This function allows external query what radio interface can be used for
 * a given RPM_FeatureType.
 *
 * Arguments:
 *     type: user requested RPM_FeatureType for which RPM_RadioInterface
 *            will be returned.
 *     *radioInfc: caller supplied object to be filled out by the routine.
 *
 * Returns:
 *     RPM_RETURN_SUCCES if no issue
 *     RPM_RETURN_ERROR: otherwise
 */
RPM_Return RPM_getAvailableRadio(
    RPM_FeatureType     type,
    RPM_RadioInterface *radioInfc_ptr)
{
    RFSM_Context *rfsmContext_ptr;

    RPM_dbgPrintf("\n");

    rfsmContext_ptr = &_RPM_objPtr->radioFsm;

    if ((RPM_FEATURE_TYPE_CALL_EMERGENCY == type) ||
            (RPM_FEATURE_TYPE_IMS_EMERGENCY_SERVICE == type)) {
        /* Emergency service and call always use emergency radio interface */
        *radioInfc_ptr = rfsmContext_ptr->emergencyRadioInfc;
    }
    else if (RPM_FEATURE_TYPE_SUPSRV == type) {
#if defined(PROVIDER_CMCC)
        /* supsrv service use operator required supsrv  */
        *radioInfc_ptr = rfsmContext_ptr->lteSupsrvRadioInfc;
#else
        /* assumed generic provider would like lte address */
        if (!OSAL_netIsAddrZero(&rfsmContext_ptr->lteRadioInfc.ipAddr)) {
            *radioInfc_ptr = rfsmContext_ptr->lteRadioInfc;
        }
        else {
            /* lte has no ip address, get current radio */
            *radioInfc_ptr = rfsmContext_ptr->radioInfc;
        }
#endif
    }
    else {
        /* Condier wifi has higher priority */
        if (!OSAL_netIsAddrZero(&rfsmContext_ptr->wifiRadioInfc.ipAddr)) {
            *radioInfc_ptr = rfsmContext_ptr->wifiRadioInfc;
        }
        /* Then see if there is lte address */
        else if (!OSAL_netIsAddrZero(&rfsmContext_ptr->lteRadioInfc.ipAddr)) {
            *radioInfc_ptr = rfsmContext_ptr->lteRadioInfc;
        }
        else {
            /* Both wifi and lte has no ip address, get current radio */
            *radioInfc_ptr = rfsmContext_ptr->radioInfc;
        }
    }

    return (RPM_RETURN_OK);
}

/*
 * ======== RPM_isEmergencyFailoverToCs() ========
 *
 * This function allows external query if an emergency call failovers to CS
 * when it failed on PS emergency call.
 *
 * Returns:
 *     OSAL_TRUE: Emergency call fails over to CS
 *     OSAL_FALSE: No failover to CS is required.
 */
OSAL_Boolean RPM_isEmergencyFailoverToCs(
    void)
{
    RPM_dbgPrintf("\n");

    return (OSAL_Boolean)(_RPM_objPtr->isEmergencyFailoverToCs);
}

/*
 * ======== RPM_isEmergencyRegRequired() ========
 *
 * This function allows external query if emergency registration is required
 * when there is a normal IMS service is active already.
 *
 * Returns:
 *     OSAL_TRUE: Emergency registration is required.
 *     OSAL_FALSE: Emergency registration is not required.
 */
OSAL_Boolean RPM_isEmergencyRegRequired(
    void)
{
    RPM_dbgPrintf("\n");

    return (OSAL_Boolean)(_RPM_objPtr->isEmergencyRegRequired);
}

/*
 * ======== RPM_setUrlFmt() ========
 *
 * This function allows external users to set url format
 * 
 *
 * Returns:
 *     none
 */
void RPM_setUrlFmt(
    RPM_UrlFmtVal intUrlFmt,
    RPM_UrlFmtVal natUrlFmt)
{
    _RPM_objPtr->urlFmt.intUrlFmt = intUrlFmt;
    _RPM_objPtr->urlFmt.natUrlFmt = natUrlFmt;
}
