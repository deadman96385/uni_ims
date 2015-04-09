/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28625 $ $Date: 2014-09-01 15:12:45 +0800 (Mon, 01 Sep 2014) $
 */

#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <osal_net.h>
#include <settings.h>
#include <ezxml_mem.h>
#include <sapp_main.h>

#ifdef INCLUDE_4G_PLUS
#include <vpad_vpmd.h>
#endif

#define D2_VPORT_REVISION D2_Release_SAPP
extern char const D2_Release_SAPP[];

#define SAPP_PROTO_DIRECTORY        "../isi/proto/sapp/"
#define SAPP_XML_DOC_PATH_NAME_SIZE (128)

/*
 * this path can optionally be set via parameter to control where
 * the OSAL queue named pipes will exist
 */
extern char OSAL_msgQPathName[OSAL_MSG_PATH_NAME_SIZE_MAX];

/*
 * ======== main ========
 *
 */
OSAL_ENTRY
{
    void *cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_SAPP);
    char *hostname_ptr = NULL;
    char  pathName[SAPP_XML_DOC_PATH_NAME_SIZE];

    if (2 > argc) {
        OSAL_logMsg(
            "\n\nStart Error --- Usage is: %s <init file> [optional <IP address> <named pipe path>]\n\n",
            argv_ptr[0]);
        return (-1);
    }

    /*
     * Optional IP.
     */
    if (argc > 2) {
        hostname_ptr = argv_ptr[2];
        OSAL_logMsg("Using <IP address>=%s\n", argv_ptr[2]);
    }

    /*
     * Optional pathname for OSAL pipes.
     */
    if (argc > 3) {
        /* leave it null terminated */
        OSAL_strncpy(OSAL_msgQPathName, argv_ptr[3], OSAL_MSG_PATH_NAME_SIZE_MAX);
        OSAL_logMsg("Using <named pipe path>=%s\n", argv_ptr[3]);
    }

    /* Register the routine to call when the process is being terminated */
    if (OSAL_SUCCESS != OSAL_condApplicationExitRegister()) {
        return (-1);
    }

    /* Init EZXML  */
    EZXML_init();

    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_SAPP,
            argv_ptr[1], cfg_ptr)) {
        /* Try to find the file under the SAPP directory */
        OSAL_snprintf(pathName, SAPP_XML_DOC_PATH_NAME_SIZE, "%s%s",
                SAPP_PROTO_DIRECTORY, argv_ptr[1]);
        if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_SAPP,
                pathName, cfg_ptr)) {
            SETTINGS_memFreeDoc(SETTINGS_TYPE_SAPP, cfg_ptr);
            OSAL_logMsg("%s: Could not find the SAPP init file %s\n",
                    __FUNCTION__, argv_ptr[1]);
            OSAL_logMsg("Copy it to the directory where your executable is.\n");
            OSAL_condApplicationExitUnregister();
            return (-1);
        }
    }

#ifdef INCLUDE_VPAD
    /* Wait for VPAD ready */
    while (OSAL_FALSE == VPAD_IS_READY()) {
        /* let's sleep for a while. */
        OSAL_logMsg("%s: VPAD is not ready, waiting...\n", __FUNCTION__);
        OSAL_taskDelay(VPAD_ERROR_RECOVERY_DELAY);
    }
#endif

    if (0 != SAPP_init(cfg_ptr, hostname_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_SAPP, cfg_ptr);
        OSAL_condApplicationExitUnregister();
        return (-1);
    }

    SETTINGS_memFreeDoc(SETTINGS_TYPE_SAPP, cfg_ptr);

    /* Block this process until it's time to terminate */
    OSAL_condApplicationExitWaitForCondition();
    OSAL_condApplicationExitUnregister();

    SAPP_shutdown();
    
    /* Destroy EZXML  */
    EZXML_destroy();
    
    return (0);
}
OSAL_EXIT

