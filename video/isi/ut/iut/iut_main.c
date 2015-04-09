/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 25442 $ $Date: 2014-04-02 13:36:58 +0800 (Wed, 02 Apr 2014) $
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include "_iut_app.h"
#include "_iut_prt.h"
#include "_iut_cfg.h"
#include "_iut_menu.h"
#include "iut_main.h"

#define D2_VPORT_REVISION D2_Release_ISI_UT
extern char const D2_Release_ISI_UT[];

/*
 * this path can optionally be set via parameter to control where
 * the OSAL queue named pipes will exist
 */
extern char OSAL_msgQPathName[OSAL_MSG_PATH_NAME_SIZE_MAX];


/* 
 * ======== _IUT_isiThread() ========
 *
 * This function is the entry point for a thread that handles events from ISI.
 * This thread waits (blocks) on ISI and processes events as they are received.
 *
 * Returns: 
 *   Nothing and never.
 */
static void _IUT_isiThread(
    void *arg_ptr)
{
    ISI_Id          serviceId;
    ISI_Id          id;
    ISI_IdType      type;
    ISI_Event       evt;
    ISI_Return      ret;
    char            eventDesc[ISI_EVENT_DESC_STRING_SZ + 1];
    
    while (1) {
        ret = ISI_getEvent(&serviceId, &id, &type, &evt, eventDesc, -1);
        if (ret == ISI_RETURN_OK) {
            /* Then the app has an event from ISI */
            
            /* Send some log message to the gui */
            IUT_prtEvent(serviceId, id, type, evt, eventDesc);
            /* Process the event */
            IUT_appProcessEvent(serviceId, id, type, evt);
            
        }
        else {
            OSAL_taskDelay(1);
        }
    }
}


/* 
 * ======== IUT_main() ========
 *
 * This function is the "main" process entry point. It initializes 
 * sub-modules, threads, and loads any configuration data needed 
 * by the ISI Unit Test application.
 *
 * Returns: 
 *   Nothing and never.
 */
OSAL_ENTRY
{
    OSAL_TaskId    task;
    
    IUT_appModuleInit();
    IUT_cfgInit();

    if (0 == (task = OSAL_taskCreate("isi", 
            OSAL_TASK_PRIO_NRT, IUT_MAIN_ISI_TASK_STACK_SZ, 
            (void *)_IUT_isiThread, 0))) {
        OSAL_logMsg("Error starting ISI thread\n");
        return (-1);
    }
    
    if (2 > argc) {
        OSAL_logMsg (
            "\n\nStart Error --- Usage is: %s <init file> [optional <named pipe path>]\n\n",
            argv_ptr[0]);
        return (-1);
    }

    /*
     * Optional pathname for OSAL pipes.
     */
    if (argc > 2) {
        /* leave it null terminated */
        OSAL_strncpy(OSAL_msgQPathName, argv_ptr[2], OSAL_MSG_PATH_NAME_SIZE_MAX);
        OSAL_logMsg("Using <named pipe path>=%s\n", argv_ptr[2]);
    }

    if (IUT_cfgRead(argv_ptr[1]) != IUT_OK) {
        OSAL_taskDelete(task);
        return (-1);
    }
    
    /* If need be launch the menu in it's own thread */
    IUT_menuMain();

    IUT_appShutdown();

    OSAL_taskDelete(task);
   
    OSAL_logMsg("Bye\n");

    return (0);
}
OSAL_EXIT
