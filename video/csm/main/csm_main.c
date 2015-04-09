/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29240 $ $Date: 2014-10-09 19:19:43 +0800 (Thu, 09 Oct 2014) $
 */

#include <osal.h>
#include <csm_main.h>
#include <settings.h>
#include <ezxml_mem.h>

#ifdef INCLUDE_VPAD
#include <vpad_vpmd.h>
#endif
#ifdef INCLUDE_VOER
#include <voer.h>
#endif

#define D2_VPORT_REVISION D2_Release_CSM
extern char const D2_Release_CSM[];

#define CSM_PROTO_DIRECTORY        "../csm/"
#define CSM_XML_DOC_PATH_NAME_SIZE (128)

int main_csmInit(int argc, char *argv_ptr[])
{
    char  pathName[CSM_XML_DOC_PATH_NAME_SIZE];
    void *cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_CSM);

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
    /* Init EZXML */
    EZXML_init();
    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_CSM,
            argv_ptr[1], cfg_ptr)) {
        /* Try to find the file under the CSM directory */
        OSAL_snprintf(pathName, CSM_XML_DOC_PATH_NAME_SIZE, "%s%s",
                CSM_PROTO_DIRECTORY, argv_ptr[1]);
        if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_CSM,
                pathName, cfg_ptr)) {
            SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, cfg_ptr);
            OSAL_logMsg("%s: Could not find the CSM init file %s\n",
                    __FUNCTION__, argv_ptr[1]);
            OSAL_logMsg("Copy it to the directory where your executable is.\n");
            OSAL_condApplicationExitUnregister();
           return (-1);
        }
    }

#ifdef INCLUDE_VPAD
    /*
     * Initialize & start VPAD
     */
    if (OSAL_SUCCESS != VPAD_init()) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, cfg_ptr);
        OSAL_condApplicationExitUnregister();
        return (-1);
    }
#endif

#ifdef INCLUDE_VOER
    /*
     * Initialize & start VOER
     */
    if (OSAL_SUCCESS != VOER_init()) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, cfg_ptr);
        OSAL_condApplicationExitUnregister();
        return (-1);
    }
#endif

    /* 
     * Initialize & start CSM 
     */
    if (0 != CSM_init(cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, cfg_ptr);
        OSAL_condApplicationExitUnregister();
        return (-1);
    }

    SETTINGS_memFreeDoc(SETTINGS_TYPE_CSM, cfg_ptr);
    /* Block this process until it's time to terminate */
    OSAL_condApplicationExitWaitForCondition();
    OSAL_condApplicationExitUnregister();

    /* Executable has exited shutdown CSM */
    CSM_shutdown();
    /* Destroy EZXML  */
    EZXML_destroy();

    return (0);
}

int main_vport4gInit(int argc, char *argv_ptr[])
{
    // char  pathName[CSM_XML_DOC_PATH_NAME_SIZE];

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
    
#ifdef INCLUDE_VPAD
    /*
     * Initialize & start VPAD
     */
    if (OSAL_SUCCESS != VPAD_init()) {
        OSAL_condApplicationExitUnregister();
        return (-1);
    }
#endif

#ifdef INCLUDE_VOER
    /*
     * Initialize & start VOER
     */
    if (OSAL_SUCCESS != VOER_init()) {
        OSAL_condApplicationExitUnregister();
        return (-1);
    }
#endif

    /*
     * Initialize & start CSM
     */
    if (2 == argc) {
        /* xxx setup path/name for config file */
    }
    if (OSAL_SUCCESS != CSM_vport4gInit()) {
        OSAL_condApplicationExitUnregister();
        return (-1);
    }

    /* Block this process until it's time to terminate */
    OSAL_condApplicationExitWaitForCondition();
    OSAL_condApplicationExitUnregister();

    /* Executable has exited shutdown CSM */
    CSM_vport4gShutdown();

    return (0);
}

/*
 * ======== csm_main ========
 *
 */
#if defined(OSAL_VXWORKS) || defined(OSAL_THREADX)
int main_csm(int argc, char *argv_ptr[])
#else
OSAL_ENTRY
#endif
{
    return main_csmInit(argc, argv_ptr);

    /* 
     * refactored vport4g init not works for 4G_PLUS yet
     *
     * return main_vport4gInit(argc,argv_ptr);
     */
}
#if (!defined(OSAL_VXWORKS)) && (!defined(OSAL_THREADX))
OSAL_EXIT
#endif
