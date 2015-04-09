/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7809 $ $Date: 2008-10-13 16:50:06 -0400 (Mon, 13 Oct 2008) $
 *
 */

/* Event reader functions
 * Assumes the VTSP has already been initialized
 */

#include "osal.h"

#include "vtsp.h"

#include "vtsp_ut.h"

OSAL_TaskId UT_eventTaskId = 0;
uvint       UT_eventTaskActive = 0;
uvint       UT_eventTaskDisplay = 1;

/* This is a task function
 * Read event queues independently in 'ANY' blocking method
 */
OSAL_TaskReturn UT_eventTaskBlock(
        OSAL_TaskArg taskArg)
{

    VTSP_EventMsg  event;
    VTSP_Return    v;

    UT_eventTaskActive = 1;

    while (1) {
        v = VTSP_getEvent(VTSP_INFC_ANY, &event, VTSP_TIMEOUT_FOREVER);

        UT_processEvent(&event);

        if (VTSP_OK == v) { 
            if (VTSP_EVENT_MSG_CODE_SHUTDOWN == event.code) {
                OSAL_logMsg("%s:%d Got SHUTDOWN event, exiting event task...\n",
                        __FILE__, __LINE__);
                break;
            }            
        }
        else if (VTSP_E_TIMEOUT == v) { 
            /* do nothing */;
            OSAL_logMsg("evTask: VTSP_E_TIMEOUT during blocking call, "
                    "VTSP error %d.\n", v);
            UT_FAILMSG;
            UT_eventTaskActive = 0;
        }
        else if ((VTSP_E_CONFIG == v) || (VTSP_E_INIT == v)) { 
            OSAL_logMsg("evTask: VTSP error %d.\n", v);
            UT_FAILMSG;
        }

        if (0 == UT_eventTaskActive) { 
            OSAL_logMsg("evTask: exiting.\n");
            break;
        }
    }

    UT_eventTaskActive = 0;
    OSAL_logMsg("evTask: exit.\n");
    return(0);
}


/* This is a task function
 * Read event queues independently in polling method
 */
vint UT_eventTaskPoll(void)
{

    VTSP_EventMsg  msg;
    VTSP_Return    v;

    UT_eventTaskActive = 1;

    while (1) {
        v = VTSP_getEvent(VTSP_INFC_GLOBAL, &msg, 20);
        if (VTSP_OK == v) { 
            if (VTSP_EVENT_MSG_CODE_SHUTDOWN == msg.code) {
                OSAL_logMsg("%s:%d Got SHUTDOWN event, exiting event task...\n",
                        __FILE__, __LINE__);
                break;
            }   
            OSAL_logMsg("evTask: recv: global event code %d\n",
                    msg.code);
        }
        else if (VTSP_E_TIMEOUT == v) { 
            /* do nothing */;
        }
        else if (VTSP_E_CONFIG == v || VTSP_E_INIT == v) { 
            OSAL_logMsg("evTask (global): VTSP error %d.\n", v);
            UT_FAILMSG;
            UT_eventTaskActive = 0;
        }

        v = VTSP_getEvent(0, &msg, 20);
        if (VTSP_OK == v) { 
            OSAL_logMsg("evTask: recv: infc event code %d\n",
                    msg.code);
        }
        else if (VTSP_E_TIMEOUT == v) { 
            /* do nothing */;
        }
        else if (VTSP_E_CONFIG == v || VTSP_E_INIT == v) { 
            OSAL_logMsg("evTask: VTSP error %d.\n", v);
            UT_FAILMSG;
            UT_eventTaskActive = 0;
        }

        if (0 == UT_eventTaskActive) { 
            OSAL_logMsg("evTask: exiting.\n");
            break;
        }
    }

    UT_eventTaskActive = 0;
    OSAL_logMsg("evTask: exit.\n");
    return(0);
}

UT_Return UT_event(void)
{
    VTSP_QueryData *vtsp_ptr;
    OSAL_TaskArg    taskArg;

    vtsp_ptr = VTSP_query();
    if (NULL == vtsp_ptr) { 
        OSAL_logMsg("%s:%d  VTSP has not been initialized\n",
                __FILE__, __LINE__);
        return (UT_FAIL);
    }

    if (0 != UT_eventTaskActive) { 
        OSAL_logMsg("%s:%d  eventTask is already running.\n",
                __FILE__, __LINE__);
    }

#ifdef OSAL_64
    taskArg.uValue = 0;
#else
    taskArg = 0;
#endif

    /*
     * ZK
     * XXX:
     * Move this ifdef to OSAL
     */
    UT_eventTaskId = OSAL_taskCreate(
            "evTask",
#if defined(OSAL_PTHREADS)
            0,         /* very low priority */
#elif defined(OSAL_WINCE)
            251,
#elif defined(OSAL_VXWORKS) || defined(OSAL_NUCLEUS)
            100,
#elif defined(OSAL_THREADX)
            OSAL_TASK_PRIO_NRT,
#else
#error PORT
#endif
            8192,
            (OSAL_TaskPtr)UT_eventTaskBlock,
            taskArg);

    if (0 == UT_eventTaskId) { 
        OSAL_logMsg("%s:%d  osal err starting UT_eventTask\n",
                __FILE__, __LINE__);
        UT_FAILMSG;
        return (UT_FAIL);
    }
    else { 
        OSAL_logMsg("evTask: is running.\n");
    }

    return (UT_PASS);
}


