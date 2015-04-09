/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 */

#include <osal.h>
#include <vpad_vpmd.h>
#include <vpr_comm.h>
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
    CSM_InputEvent  event;
    OSAL_NetAddress addr;
#ifndef RIR_ENABLE_IPC_MSG_Q
    VPR_Comm        comm;
#endif

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

#ifdef RIR_ENABLE_IPC_MSG_Q
    OSAL_msgQSend(obj_ptr->csmEvtInQ, (void *)&event,
            sizeof(CSM_InputEvent), OSAL_NO_WAIT, NULL);
#else

    /* Send to VPR through VPAD */
    comm.type = VPR_TYPE_CSM_EVT;
    OSAL_memCpy(&comm.u.csmEvt, &event, sizeof(CSM_InputEvent));
    VPAD_writeCsmEvt(&comm, sizeof(VPR_Comm));
#endif
    _RIR_logMsg("%s %d: Protocol %s notified of new IP\n",
            __FILE__, __LINE__, CSM_INPUT_EVENT_QUEUE_NAME);
}
