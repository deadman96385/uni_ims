/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 */

#include <osal.h>
#include "../csm/csm_event.h"

#include "_rir.h"
#include "_rir_log.h"

/*
 * ======== _RIR_notifyProtos() ========
 *
 * Notify protocols that a change of IP has occured.
 * List of protocol queue names comma separated in protos_ptr.
 *
 * Returns:
 *
 */
void _RIR_notifyProtos(
    _RIR_Obj    *obj_ptr,
    RIR_Command *cmd_ptr)
{
    CSM_InputEvent      event;
    OSAL_NetAddress     addr;
    CSM_InputService   *service_ptr;

    OSAL_logMsg("%s:%d\n", __FUNCTION__, __LINE__);

    /*
     * Send BSSID info event to CSM_Event
     */
    if (0 == _RIR_isMacAddrZero(cmd_ptr->bssid)) {
        event.type = CSM_EVENT_TYPE_SERVICE;
        service_ptr = &event.evt.service;
        OSAL_snprintf(service_ptr->u.cgi.id, sizeof(service_ptr->u.cgi.id),
                "%02x%02x%02x%02x%02x%02x",
                cmd_ptr->bssid[0], cmd_ptr->bssid[1], cmd_ptr->bssid[2],
                cmd_ptr->bssid[3], cmd_ptr->bssid[4], cmd_ptr->bssid[5]);
        service_ptr->reason = CSM_SERVICE_REASON_UPDATE_CGI;
        service_ptr->u.cgi.type = CSM_SERVICE_ACCESS_TYPE_IEEE_802_11;
        OSAL_strncpy(service_ptr->reasonDesc, "CSM_SERVICE_REASON_UPDATE_CGI",
                sizeof(service_ptr->reasonDesc));
        OSAL_msgQSend(obj_ptr->csmEvtInQ, (void *)&event,
                sizeof(CSM_InputEvent), OSAL_NO_WAIT, NULL);

    }

    /* Send the IP change event to CSM_Event */
    event.type = CSM_EVENT_TYPE_RADIO;
    event.evt.radio.reason = CSM_RADIO_REASON_IP_CHANGE;

    /* Convert to host byte order string */
    OSAL_netAddrNtoh(&addr, &cmd_ptr->addr);
    OSAL_netAddressToString(event.evt.radio.address, &addr);

    OSAL_strncpy(event.evt.radio.infcName, cmd_ptr->infcName,
            sizeof(event.evt.radio.infcName));

    if (0 == OSAL_strncasecmp(cmd_ptr->typeName, RIR_NETWORK_TYPE_NAME_WIFI,
            sizeof(cmd_ptr->typeName))) {
        event.evt.radio.networkType = CSM_RADIO_NETWORK_TYPE_WIFI;
    }
    else if (0 == OSAL_strncasecmp(cmd_ptr->typeName,
            RIR_NETWORK_TYPE_NAME_LTE,
            sizeof(cmd_ptr->typeName))) {
        event.evt.radio.networkType = CSM_RADIO_NETWORK_TYPE_LTE;
    }
    else {
        event.evt.radio.networkType = CSM_RADIO_NETWORK_TYPE_NONE;
    }

    /*
     * Send new IP info.
     */
    OSAL_msgQSend(obj_ptr->csmEvtInQ, (void *)&event,
            sizeof(CSM_InputEvent), OSAL_NO_WAIT, NULL);
    _RIR_logMsg("%s %d: Protocol %s notified of new IP\n",
            __FILE__, __LINE__, CSM_INPUT_EVENT_QUEUE_NAME);

}
