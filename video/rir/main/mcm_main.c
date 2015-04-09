/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 */

#include <osal.h>
#include <ezxml_mem.h>
#include "rir.h"
#include <settings.h>

#define D2_VPORT_REVISION D2_Release_RIR
extern char const D2_Release_RIR[];

#ifdef RIR_MANUAL_IP
#include <vpad_vpmd.h>
#include <vpr_comm.h>
#include "../csm/csm_event.h"

/*
 * ======== _RIR_manualConfig() ========
 *
 * Send the IP address string to csm event for manually config IP
 *
 * Returns:
 *
 */
static void _RIR_manualConfig(
    char *str_ptr,
    vint radioType)
{
    CSM_InputEvent  event;
    VPR_Comm        comm;

    OSAL_logMsg("IP address is %s\n", str_ptr);

    /* Send the IP change event to CSM_Event */
    event.type = CSM_EVENT_TYPE_RADIO;
    event.evt.radio.reason = CSM_RADIO_REASON_IP_CHANGE;

    OSAL_strncpy(event.evt.radio.address, str_ptr,
            sizeof(event.evt.radio.address));

    OSAL_strncpy(event.evt.radio.infcName, "lte",
            sizeof(event.evt.radio.infcName));

    event.evt.radio.networkType = radioType;

    /* Send to VPR through VPAD */
    comm.type = VPR_TYPE_CSM_EVT;
    OSAL_memCpy(&comm.u.csmEvt, &event, sizeof(CSM_InputEvent));
    if (OSAL_SUCCESS != VPAD_writeCsmEvt(&comm, sizeof(VPR_Comm))) {
        OSAL_logMsg("Write CSM event thru VPAD failed\n");
    } else {
        OSAL_logMsg("Done writen CSM event thru VPAD\n");
    }
}
#endif /* RIR_MANUAL_IP */


/*
 * ======== main() ========
 
 * MCM Entry point.
 * Start with OSAL conventions. 
 * 
 * Returns:
 *  -1 : Failed
 *   0 : OK
 */
OSAL_ENTRY
{
    void *cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_RIR);

#ifdef RIR_MANUAL_IP
    if (2 > argc) {
        OSAL_logMsg(
            "\n\nStart Error --- Usage is: %s <xml-path>|<IP address> [radio-type]\n\n",
            argv_ptr[0]);
        OSAL_logMsg("radio type: 0-3 = none,lte,wifi,lte_ss\n\n");
        return (-1);
    }

    if (argv_ptr[1][0] >= '0' && argv_ptr[1][0] <= '9') {
        /* manually config ip address for LTE */
        if (3 == argc) {
            _RIR_manualConfig(argv_ptr[1], OSAL_atoi(argv_ptr[2]));
        } else {
            _RIR_manualConfig(argv_ptr[1], CSM_RADIO_NETWORK_TYPE_LTE);
        }
        return (0);
    }
#endif /* RIR_MANUAL_IP */

    if (OSAL_SUCCESS != OSAL_condApplicationExitRegister()) {
        return (-1);
    }
    /* Init EZXML */
    EZXML_init();

    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_RIR,
            argv_ptr[1], cfg_ptr)) {
        OSAL_logMsg("%s: Could not find the RIR init file %s\n",
                __FUNCTION__, argv_ptr[1]);
        SETTINGS_memFreeDoc(SETTINGS_TYPE_RIR, cfg_ptr);
        return (-1);
    }
    
    if (0 != RIR_init(cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_RIR, cfg_ptr);
        OSAL_logMsg("Failed to init RIR!\n");
        return (-1);
    }
    // Free the buffer
    SETTINGS_memFreeDoc(SETTINGS_TYPE_RIR, cfg_ptr);
    
    OSAL_condApplicationExitWaitForCondition();
    OSAL_condApplicationExitUnregister();
    /* Destroy EZXML */
    EZXML_destroy();
    RIR_shutdown();

    OSAL_logMsg("RIR finished.\n");

    return (0);
}
OSAL_EXIT
