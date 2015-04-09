/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28271 $ $Date: 2014-08-18 15:24:33 +0800 (Mon, 18 Aug 2014) $
 */

#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <osal_net.h>
#include <mc_main.h>
#include <settings.h>
#include <ezxml_mem.h>

#define MC_PROTO_DIRECTORY        "../isi/proto/mc/"
#define MC_XML_DOC_PATH_NAME_SIZE (128)

#define D2_VPORT_REVISION D2_Release_MC
extern char const D2_Release_MC[];

/*
 * ======== main ========
 *
 */
OSAL_ENTRY
{
    void *cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_MC);
    char  pathName[MC_XML_DOC_PATH_NAME_SIZE];

    if (2 > argc) {
        OSAL_logMsg (
            "\n\nStart Error --- Usage is: %s <init file>\n\n",
            argv_ptr[0]);
        return (-1);
    }
    
    /* Register the routine to call when the process is being terminated */
    if (OSAL_SUCCESS != OSAL_condApplicationExitRegister()) {
        return (-1);
    }

    /* Init EZXML  */
    EZXML_init();

    /* Get the XML init info */
    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_MC,
            argv_ptr[1], cfg_ptr)) {
        OSAL_snprintf(pathName, MC_XML_DOC_PATH_NAME_SIZE, "%s%s",
                MC_PROTO_DIRECTORY, argv_ptr[1]);
        if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_SAPP,
                pathName, cfg_ptr)) {
            SETTINGS_memFreeDoc(cfg_ptr);
            OSAL_logMsg("%s: Could not find the MC init file %s\n",
                    __FUNCTION__, argv_ptr[1]);
            OSAL_logMsg("Copy it to the directory where your executable is.\n");
            OSAL_condApplicationExitUnregister();
            return (-1);
        }
    }

    if (0 != MC_init(cfg_ptr)) {
        SETTINGS_memFreeDoc(cfg_ptr);
        OSAL_condApplicationExitUnregister();
        return (-1);
    }

    SETTINGS_memFreeDoc(cfg_ptr);
    /* Destroy EZXML  */
    EZXML_destroy();


    /* Block this process until it's time to terminate */
    OSAL_condApplicationExitWaitForCondition();
    OSAL_condApplicationExitUnregister();

    MC_shutdown();
    return (0);
}
OSAL_EXIT
