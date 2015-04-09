/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 18034 $ $Date: 2012-08-29 10:57:34 -0700 (Wed, 29 Aug 2012) $
 */

#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <osal_net.h>
#include <ezxml_mem.h>
#include <gapp_main.h>
#include <settings.h>

#define D2_VPORT_REVISION "D2 GAPP"
//#define D2_VPORT_REVISION D2_Release_GAPP
//extern char const D2_Release_GAPP[];

/*
 * ======== main ========
 *
 */
OSAL_ENTRY
{
    void *cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_GAPP);

    if (2 > argc) {
        OSAL_logMsg (
          "\n\nStart Error --- Usage is: %s <init file> [<FMC init file>]\n\n",
          argv_ptr[0]);
        return (-1);
    }

    /* Register the routine to call when the process is being terminated */
    if (OSAL_SUCCESS != OSAL_condApplicationExitRegister()) {
        return (-1);
    }
 
    /* Init EZXML  */
    EZXML_init();

    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_GAPP,
            argv_ptr[1], cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_GAPP, cfg_ptr);
        OSAL_logMsg("%s: Could not find the GAPP init file %s\n",
                __FUNCTION__, argv_ptr[1]);
        return (-1);
    }

    if (0 != GAPP_init(cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_GAPP, cfg_ptr);
        OSAL_condApplicationExitUnregister();
        return (-1);
    }
    SETTINGS_memFreeDoc(SETTINGS_TYPE_GAPP, cfg_ptr);

    /* Destroy EZXML  */
    EZXML_destroy();

    /* Block this process until it's time to terminate */
    OSAL_condApplicationExitWaitForCondition();
    OSAL_condApplicationExitUnregister();

    GAPP_shutdown();
    return (0);
}
OSAL_EXIT

