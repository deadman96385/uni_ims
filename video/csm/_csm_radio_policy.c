/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29954 $ $Date: 2014-11-20 10:39:55 +0800 (Thu, 20 Nov 2014) $
 *
 */
#include <isi.h>
#include <rpm.h>
#include <csm_event.h>
#include "_csm_radio_policy.h"
#include "_csm.h"
#ifdef INCLUDE_SUPSRV
#include "_csm_supsrv.h"
#endif
/*
 * ======== CSM_rpmInit() ========
 *
 * Public initializer routine for the CSM Radio Policy Manager (RPM) package
 *
 * Returns: 
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint CSM_rpmInit(
    CSM_ServiceMngr    *serviceMngr_ptr)
{
    CSM_dbgPrintf("\n");

    /* Initialize the RPM library */
    RPM_init();
    /* Set  Provisioning URL format setting to RPM */
    RPM_setUrlFmt((RPM_UrlFmtVal)serviceMngr_ptr->urlFmt.intUrlFmt, 
            (RPM_UrlFmtVal)serviceMngr_ptr->urlFmt.natUrlFmt);
    /* Setup the callback for SRVCC to CallManager */
    RPM_registerImsRadioChangeCallback(CSM_serviceOnImsRadioChange);

    /* Setup the callback for ImsRadio to ServiceManager */
    RPM_registerSrvccChangeCallback(CSM_callsOnSrvccChange);

    /* Setup the callback for emergency registraion required
     * to ServiceManager.
     */
    RPM_eRegisterRequiredChangeCallback(CSM_serviceEmerRegRequiredChange);

#ifdef INCLUDE_SUPSRV
    /* Setup policy for SupSrv access */
    RPM_registerSupsrvRadioChangeCallback(CSM_supSrvOnRadioChange);
#endif

    return (CSM_OK);
}

/*
 * ======== CSM_rpmProcessEvent() ========
 *
 * Event entry point into the RPM Package.
 *
 * Returns: 
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint CSM_rpmProcessEvent(
    CSM_InputRadio *radioEvt_ptr)
{
    RPM_Event       event;
    OSAL_NetAddress addr;

    CSM_dbgPrintf("\n");

    switch (radioEvt_ptr->reason) {
        case CSM_RADIO_REASON_IP_CHANGE:
            CSM_dbgPrintf("CSM_EVENT_RADIO_REASON_IP_CHANGE ip: %s networkType:%d\n",
                    radioEvt_ptr->address, radioEvt_ptr->networkType);
            /* Convert this to an RPM event and pass to RPM library */
            event.type = RPM_EVENT_TYPE_IP_CHANGE;
            if (CSM_RADIO_NETWORK_TYPE_LTE == radioEvt_ptr->networkType) {
                event.radioType = RPM_RADIO_TYPE_LTE;
            }
            else if (CSM_RADIO_NETWORK_TYPE_WIFI == radioEvt_ptr->networkType) {
                event.radioType = RPM_RADIO_TYPE_WIFI;
            }
            else if (CSM_RADIO_NETWORK_TYPE_LTE_SS == radioEvt_ptr->networkType) {
                event.radioType = RPM_RADIO_TYPE_LTE_SS;
            }
            else {
                /* do not care other interface mode. */
                break;
            }

            OSAL_netStringToAddress((int8 *)radioEvt_ptr->address, &addr);
            /* Convert to host byte order. */
            OSAL_netAddrNtoh(&event.u.ipAddress, &addr);
            RPM_processEvent(&event);
            break;
        case CSM_RADIO_REASON_IP_CHANGE_EMERGENCY:
            CSM_dbgPrintf("CSM_EVENT_RADIO_REASON_IP_CHANGE_EMERGENCY ip: %s\n",
                    radioEvt_ptr->address);
            /* Convert this to an RPM event and pass to RPM library */
            event.type = RPM_EVENT_TYPE_IP_CHANGE;
            event.radioType = RPM_RADIO_TYPE_LTE_EMERGENCY;
            OSAL_netStringToAddress((int8 *)radioEvt_ptr->address, &addr);
            /* Convert to host byte order. */
            OSAL_netAddrNtoh(&event.u.ipAddress, &addr);
            RPM_processEvent(&event);
            break;
        case CSM_RADIO_REASON_RADIO_CHANGE_EMERGENCY:
            /* Emergency radio related configurations changed */
            CSM_dbgPrintf("CSM_RADIO_REASON_RADIO_CHANGE_EMERGENCY: "
                    "isFailoverToCs:%d, isEmergencyRegRequired:%d\n",
                    radioEvt_ptr->isEmergencyFailoverToCs,
                    radioEvt_ptr->isEmergencyRegRequired);
            event.type = RPM_EVENT_TYPE_SERVICE_CHANGE;
            event.radioType = RPM_RADIO_TYPE_LTE_EMERGENCY;
            event.u.config.isEmergencyFailoverToCs =
                    radioEvt_ptr->isEmergencyFailoverToCs;
            event.u.config.isEmergencyRegRequired = 
                    radioEvt_ptr->isEmergencyRegRequired;
            RPM_processEvent(&event);
            break;
        default:
            break;
    }

    return (CSM_OK);
}

/*
 * ======== CSM_rpmShutdown() ========
 *
 * Public shutdown routine for the CSM Radio Policy Manager (RPM) package
 *
 * Returns: 
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint CSM_rpmShutdown(
    void)
{
    CSM_dbgPrintf("\n");

    /* Shutdown RPM */
    RPM_shutdown();

    return (CSM_OK);
}
