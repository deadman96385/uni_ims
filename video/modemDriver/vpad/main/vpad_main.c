/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 */

#include <osal.h>
#include <vpad_vpmd.h>


#define D2_VPORT_REVISION "D2_R8"//D2_Release_MODEM_DRVR_VPAD
extern char const D2_Release_MODEM_DRVR_VPAD[];

/*
 * ======== vpad_main ========
 *
 */
OSAL_ENTRY
{
    /* Register the routine to call when the process is being terminated */
    if (OSAL_SUCCESS != OSAL_condApplicationExitRegister()) {
        return (-1);
    }

    /*
     * Initialize & start VPAD
     */
    VPAD_init();

    /* Block this process until it's time to terminate */
    OSAL_condApplicationExitWaitForCondition();
    OSAL_condApplicationExitUnregister();

    /* Executable has exited shutdown VPAD */
    VPAD_destroy();
    return (0);
}
OSAL_EXIT

